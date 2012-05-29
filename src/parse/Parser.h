// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

// Reads from a TokenBuffer and builds an AST, possibly producing warnings and errors
#ifndef HUE__PARSER_H
#define HUE__PARSER_H

#include "TokenBuffer.h"
#include "../Logger.h"
#include <hue/ast/ast.h>

#include <vector>

#define DEBUG_PARSER 0
#if DEBUG_PARSER
  #include "../DebugTrace.h"
  #define DEBUG_TRACE_PARSER DEBUG_TRACE
#else
  #define DEBUG_TRACE_PARSER do{}while(0)
#endif


namespace hue {
using namespace ast;


// Precedence of the pending binary operator token.
// This acts as a parse tree terminator -- when this returns -1, whatever expression
// is being parsed has reached its end and the parse branch ends/returns.
static int BinaryOperatorPrecedence(const Token& token) {
  // Modeled after JS op precedence, see:
  // https://developer.mozilla.org/en/JavaScript/Reference/Operators/Operator_Precedence
  if (token.type == Token::BinaryOperator) {
    switch (token.textValue[0]) {
      // TODO: unary logical-not '!'
      //       unary + '+'
      //       unary negation '-'
      //       
      case '*': case '/': return 100; //todo: '%' ?
      case '+': case '-': return 90;
      // todo: bitwise shift?
      case '<': case '>': return 70;
      // equality ops (in separate branch for Token::BinaryComparisonOperator)
      case '&': return 50; // Logical AND (C equiv: '&&')
      case '|': return 40; // Logical OR  (C equiv: '||')
      
      case '=': return 30;
      
      default: return -1;
    }
  } else if (token.type == Token::BinaryComparisonOperator) {
    // '<=' '>=' '!=' '=='
    // We only check first byte since second byte is always '='
    switch (token.textValue[0]) {
      case '<': case '>': return 70;
      case '=': case '!': return 60;
      default: return -1;
    }
  }
  return -1;
}


template <typename T>
class ScopeFlag {
  T *value_;
  T originalValue_;
public:
  ScopeFlag(T *value, T newValue) : value_(value) {
    originalValue_ = *value_;
    *value_ = newValue;
  }
  ~ScopeFlag() {
    *value_ = originalValue_;
  }
};


class Parser {
  typedef uint32_t LineLevel;
  static const LineLevel RootLineLevel = UINT32_MAX;
  static const LineLevel InferLineLevel = UINT32_MAX-1;
  TokenBuffer& tokens_;
  Token token_;
  Token previousToken_;
  Token& futureToken_;
  bool isParsingCallArguments_ = false;
  LineLevel previousLineLevel_ = 0;
  LineLevel currentLineLevel_ = 0;
  
  std::vector<Token> recentComments_;
  std::vector<std::string> errors_;
  //std::vector<std::string> warnings_;
  //std::vector<std::string> notices_;
  
public:
  explicit Parser(TokenBuffer& tokens)
    : tokens_(tokens)
    , token_(NullToken)
    , previousToken_()
    , futureToken_(const_cast<Token&>(NullToken)) {}
  
  // ------------------------------------------------------------------------
  // Error handling
  Expression *error(const std::string& str) {
    std::ostringstream ss;
    ss << str << " (" << token_.toString() << ")";
    errors_.push_back(ss.str());
    
    #if DEBUG_PARSER
    fprintf(stderr, "\e[31;1mError: %s\e[0m (%s)\n Token trace:\n", str.c_str(), token_.toString().c_str());
    // list token trace
    size_t n = 1; // excluding the futureToken_
    while (n < tokens_.count()) {
      if (tokens_[n].type == Token::NewLine && tokens_[n].line == 6) {
        std::cerr << "  \e[31;1m" << tokens_[n++].toString() << "\e[0m" << std::endl;
      } else
      std::cerr << "  " << tokens_[n++].toString() << std::endl;
    }
    #else
    fprintf(stderr, "\e[31;1mError: %s\e[0m (%s)\n", str.c_str(), token_.toString().c_str());
    #endif
    
    return 0;
  }
  
  const std::vector<std::string>& errors() const { return errors_; };
  
  bool tokenTerminatesCall(const Token& token) const {
    return token.type != Token::Identifier
        && token.type != Token::IntLiteral
        && token.type != Token::FloatLiteral
        && token.type != Token::DataLiteral
        && token.type != Token::TextLiteral
        && token.type != Token::LeftParen
        && token.type != Token::If
        && (token.type != Token::NewLine || currentLineLevel_ <= previousLineLevel_);
  }
  
  // bool tokenIsType() const {
  //   return token_.type == Token::Identifier
  //       || token_.type == Token::IntSymbol
  //       || token_.type == Token::FloatSymbol
  //       || token_.type == Token::Bool
  //       || token_.type == Token::Func;
  // }
  
  bool tokenIsCommonSeparator() const {
    return token_.type == Token::Colon
        || token_.type == Token::Comma
        || token_.type == Token::End
        || token_.type == Token::NewLine
        || token_.type == Token::Assignment
        || token_.type == Token::Semicolon;
  }
  
  // ------------------------------------------------------------------------
  
  // Variable = Identifier 'MUTABLE'? Type?
  Variable *parseVariable(const Text& identifierName) {
    DEBUG_TRACE_PARSER;
    Type *T = NULL;
    bool isMutable = false;
    
    if (token_.type == Token::Mutable) {
      isMutable = true;
      nextToken(); // eat 'MUTABLE'
    }
    
    //if (tokenIsType()) {
    if (!tokenIsCommonSeparator()) {
      T = parseType();
      if (!T) return 0;
    }
    
    return new Variable(isMutable, identifierName, T);
  }
  
  // VariableList = (Variable ',')* Variable
  //
  // Examples:
  //   x
  //   x Int, 
  //   x, y Int, foo [Byte]
  //
  VariableList *parseVariableList(Text firstVarIdentifierName = Text()) {
    DEBUG_TRACE_PARSER;
    VariableList *varList = new VariableList();
    bool useArg0 = !firstVarIdentifierName.empty();
    
    while (1) {
      // var
      Variable *variable;
      
      if (useArg0) {
        variable = parseVariable(firstVarIdentifierName);
        useArg0 = false;
      } else {
        Text identifierName = token_.textValue;
        nextToken(); // eat id
        variable = parseVariable(token_.textValue);
      }
      
      if (variable == 0) return 0; // TODO: cleanup
      varList->push_back(variable);
    
      // If we get a comma here, there's another variable to parse, otherwise we're done
      if (token_.type != Token::Comma) break;
      nextToken(); // eat ','
      
      if (token_.type != Token::Identifier) {
        error("Expected variable identifier");
        nextToken(); // Skip token for error recovery.
        return 0; // TODO: cleanup
      }
    }
    
    return varList;
  }
  
  // Helper function that verifies the intergrity of a variable list and expands types.
  VariableList* verifyAndUpdateVariableList(VariableList* varList) {
    assert(varList != 0);
    VariableList::const_reverse_iterator rit = varList->rbegin();
    VariableList::const_reverse_iterator rend = varList->rend();
    Type *currentT = 0;
    for (; rit != rend; ++rit) {
      Variable* var = *rit;
      if (var->hasUnknownType()) {
        if (currentT == 0) {
          error("Malformed variable list (type declaration is missing from last variable)");
          return 0;
        }
        var->setType(currentT);
      } else {
        currentT = var->type();
      }
    }
    return varList;
  }
  
  /// callexpr
  ///   ::= expr*
  // Examples:
  //   'foo 1 2.3 x' (C equiv: foo(1, 2.3, x))
  // foo 10 x (foo (foo 20 y) 30)
  //
  // foo a              -- call function "foo" with argument symbol "a"
  // foo a b = 4        -- error cant assign to a function call (more than two identifiers)
  // foo 4 b = 4        -- error cant assign to a function call (more than two identifiers)
  // foo = 4            -- define symbol "foo" of inferred type as value 4
  // foo a = 4          -- define symbol "foo" of type "a" as value 4
  // foo MUTABLE a = 4  -- define variable "foo" of type "a" as value 4
  // foo a              -- call function "foo" with argument symbol "a"
  // foo 4 = 4          -- error cant assign to a function call (number literal is not an identifier)
  //
  // foo a b (c = x d)  -->  foo(a, b, (c = x(d)))
  //
  Expression *parseCall(Text identifierName) {
    DEBUG_TRACE_PARSER;
    
    ScopeFlag<bool> isParsingCallArguments(&isParsingCallArguments_, true);
    
    // Read call argument values: expression*
    std::vector<Expression*> args;

    do {
      Expression *arg;
      
      if (token_.type == Token::LeftParen) {
        // a subexpression
        ScopeFlag<bool> isParsingCallArguments2(&isParsingCallArguments_, false);
        nextToken();
        arg = parseExpression();
        if (!arg) return 0;
        
        // Require argument terminator: ')'
        if (token_.type != Token::RightParen) {
          return error("Expected ')' after subexpression");
        }
        nextToken();  // eat ')'.
      
      } else if (token_.type == Token::NewLine) {
        break;
      } else {
        arg = parseExpression();
        if (!arg) return 0;
      }
      
      args.push_back(arg);
    } while (token_.type == Token::LeftParen || !tokenTerminatesCall(token_.type));
    
    //printf("Call to '%s' w/ %lu args\n", identifierName.c_str(), args.size());
    return new Call(identifierName, args);
  }
  
  
  // IdentifierExpr = Identifier (= | Expression+)?
  Expression *parseIdentifierExpr() {
    DEBUG_TRACE_PARSER;
    Text identifierName = token_.textValue;
    nextToken();  // eat identifier.
    
    // TODO: Refactor this mess...
    
    if (   token_.type       == Token::Assignment
        || token_.type       == Token::Mutable
        || futureToken_.type == Token::Assignment
        || token_.type       == Token::LeftSqBracket ) {

      // if (token_.type == Token::Assignment) {
      //   nextToken(); // Eat '='
      // }
      // Look-ahead to solve the case: foo Bar = ... vs foo Bar baz (call)
      return parseAssignment(identifierName);
    } if (isParsingCallArguments_ || tokenTerminatesCall(token_)) {
       return new Symbol(identifierName);
    }
    
    return parseCall(identifierName);
  }
  
  
  // Assignment = Variable '=' Expression
  Assignment *parseAssignment(Text firstVarIdentifierName) {
    DEBUG_TRACE_PARSER;

    Variable *variable = parseVariable(firstVarIdentifierName);
    if (!variable) return 0;
    
    if (token_.type != Token::Assignment) {
      error("Expected assignment operator");
      nextToken(); // Skip token for error recovery.
      return 0; // TODO: cleanup
    }
    nextToken(); // eat '='
    
    // After '='
    Expression *rhs = parseExpression();
    if (rhs == 0) {
      return 0; // TODO: cleanup
    }
    
    return new Assignment(variable, rhs);
  }
  
  
  /// binop_rhs
  ///   ::= (op primary)*
  Expression *parseBinOpRHS(int lhsPrecedence, Expression *lhs) {
    DEBUG_TRACE_PARSER;
    // If this is a binop, find its precedence.
    while (1) {
      int precedence = BinaryOperatorPrecedence(token_);
    
      // If this is a binop that binds at least as tightly as the current binop,
      // consume it, otherwise we are done.
      if (precedence < lhsPrecedence)
        return lhs;
    
      // Okay, we know this is a binop.
      char binOperator = token_.textValue[0];
      BinaryOp::Kind binKind = BinaryOp::SimpleLTR;
      if (token_.type == Token::BinaryComparisonOperator) {
        binKind = BinaryOp::EqualityLTR;
      }
      
      nextToken();  // eat binop
    
      // Parse the primary expression after the binary operator.
      Expression *rhs = parsePrimary();
      if (!rhs) return 0;
    
      // If BinOp binds less tightly with RHS than the operator after RHS, let
      // the pending operator take RHS as its LHS.
      int nextPrecedence = BinaryOperatorPrecedence(token_);
      if (precedence < nextPrecedence) {
        rhs = parseBinOpRHS(precedence+1, rhs);
        if (rhs == 0) return 0;
      }
    
      // Merge LHS and RHS
      lhs = new BinaryOp(binOperator, lhs, rhs, binKind);
    }
  }
  
  // Type = ArrayType | PrimitiveType
  // ArrayType = '[' PrimitiveType ']'
  // PrimitiveType = 'Int' | 'Float' | 'func' | 'extern' | Identifier
  Type *parseType() {
    DEBUG_TRACE_PARSER;
    Type* T = 0;
    
    // Array? '[' subtype ']'
    if (token_.type == Token::LeftSqBracket) {
      nextToken(); // Eat '['
      
      // Parse subtype
      T = parseType();
      if (T == 0) return 0;
      
      // Expect ']'
      if (token_.type != Token::RightSqBracket) {
        error("Expected terminating ']' after array type");
        delete T;
        return 0;
      }
      nextToken(); // Eat ']'
      
      return new ArrayType(T);
    }
    
         if (token_.type == Token::IntSymbol)   T = new Type(Type::Int);
    else if (token_.type == Token::FloatSymbol) T = new Type(Type::Float);
    else if (token_.type == Token::Func)        T = new Type(Type::Func);
    else if (token_.type == Token::Bool)        T = new Type(Type::Bool);
    else if (token_.type == Token::Byte)        T = new Type(Type::Byte);
    else if (token_.type == Token::Char)        T = new Type(Type::Char);
    else if (token_.type == Token::Identifier)  T = new Type(token_.textValue);
    else error("Unexpected token while expecting type identifier");
    
    nextToken(); // eat token
    
    return T;
  }
  
  // TypeList = (Type ',')* Type
  //
  // Examples:
  //   Int
  //   Int, Float, foo
  //
  TypeList *parseTypeList() {
    TypeList *typeList = new TypeList();
    
    while (1) {
      Type *type = parseType();
      if (!type) {
        delete typeList; // todo: delete contents
        return 0;
      }
      
      typeList->push_back(type);
      
      // Comma means there are more types
      if (token_.type != Token::Comma) break;
      nextToken(); // eat ','
    }
    
    return typeList;
  }
  
  
  // A set of epxressions where the last expressions' result becomes the
  // block's result.
  //
  // BlockExpression = Expression+
  // 
  // Expressions are read as part of the block as long as:
  //
  //  In the case of a root block:
  //    - There are valid tokens (any but End, Semicolon and Unexpected)
  //  In the case of a non-root block:
  //    -   line indentation > outer line indentation
  //    or: token is Semicolon
  //    or: token is End or Unexpected
  // 
  Block* parseBlock() {
    DEBUG_TRACE_PARSER;
    Block* block = new Block();
    //uint32_t startLine = token_.line;
    uint32_t baseColumn = UINT32_MAX;
    
    while (token_.type != Token::End && token_.type != Token::Error && token_.type != Token::Unexpected) {

      // Consume any newlines
      while (token_.type == Token::NewLine) { nextToken(); }

      // Find base column?
      if (baseColumn == UINT32_MAX) {
        baseColumn = token_.column;
        #if DEBUG_PARSER
        rlog("Block's baseColumn set to " << baseColumn);
        #endif
      }
      
      // Read one expression
      Expression *expr = parseExpression();
      if (expr == 0) {
        delete block; // TODO: cleanup
        return 0;
      }
      block->addExpression(expr);

      // if x 123
      //      456
      //   789
      // --> (123, 456)
      //
      // if x
      //  123    <-- First expression denotes line level
      //  456
      //   789
      // --> (123, 456, 789)
      //
      
      // Read any linebreaks and check the line level after each linebreak, starting with the one
      // we just read.
      if (token_.type == Token::NewLine || previousToken_.type == Token::NewLine) {

        // Consume any newlines
        while (token_.type == Token::NewLine) { nextToken(); }
        
        // Dropped of level?
        if (token_.column < baseColumn) {
          #if DEBUG_PARSER
          rlog("Block ended (line level drop)");
          #endif
          break;
        }
      
      // We might have dropped our level already (if parseExpression() just returned from
      // a block inside this block).
      } else {
        #if DEBUG_PARSER
        rlog("Block ended (something else following an expression)");
        rlog("  token_: " << token_.toString());
        #endif
        break;
      }
      
    }
    
    return block;
  }
  

  // Entry points:
  //   FunctionExpr   = "func" FunctionType
  //   ExternalExpr   = "external" Identifier FunctionType
  //
  // FunctionType     = Parameters? Result?
  //   Result         = Type
  //   Parameters     = "(" VariableList? ")"
  //
  // Examples:
  //   func
  //   func (x Int)
  //   func (x, y Int) Float
  //   func Float
  //   func (x, y Int, z Float) Int, Float
  //   extern atan2 (x, y Float) Float
  //
  FunctionType *parseFunctionType(bool expectResultType) {
    DEBUG_TRACE_PARSER;
    
    VariableList *variableList = NULL;
    Type* returnType = 0;
  
    // Parameters =
    if (token_.type == Token::LeftParen) {
      // '('
      nextToken(); // eat '('
      // VariableList =
      variableList = parseVariableList();
      if (variableList == 0) return 0;
    
      // ')'
      if (token_.type != Token::RightParen) {
        return (FunctionType*)error("Expected ')' after function parameters");
      }
      nextToken();  // eat ')'.
      
      variableList = verifyAndUpdateVariableList(variableList);
      if (variableList == 0) return 0;
    }
    
    // Result =
    if (!expectResultType) {
      returnType = new Type(Type::Unknown);
    } else if (!tokenIsCommonSeparator()) {
      returnType = parseType();
      if (returnType == 0) {
        if (variableList) delete variableList;
        return 0;
      }
    }
  
    // Create function interface
    return new FunctionType(variableList, returnType);
  }
  
  
  // Function = 'func' FunctionType ':' BlockExpression
  Function *parseFunction() {
    DEBUG_TRACE_PARSER;
    //LineLevel funcLineLevel = currentLineLevel_;//token_.column;
    nextToken();  // eat 'func'
    
    // Parse function interface
    FunctionType *interface = parseFunctionType(false);
    if (interface == 0) return 0;
    
    // Parse body
    Block* body = parseBlock();
    if (body == 0) return 0;
    
    // Create function node
    return new Function(interface, body);
  }
  

  /// external ::= 'extern' id func_interface linebreak
  ExternalFunction *parseExternalFunction() {
    DEBUG_TRACE_PARSER;
    nextToken();  // eat 'extern'
    
    if (token_.type != Token::Identifier) {
      return (ExternalFunction*)error("Expected name after 'extern'");
    }
    
    // Remember id
    Text funcName = token_.textValue;
    nextToken(); // eat id
    
    FunctionType *funcInterface = parseFunctionType(true);
    if (!funcInterface) return 0;
    
    // External functions are always public
    funcInterface->setIsPublic(true);
    
    // Require terminating linebreak
    if (token_.type != Token::NewLine) {
      delete funcInterface;
      return (ExternalFunction*)error("Expected linebreak after external declaration");
    }
    nextToken(); // eat linebreak
    
    return new ExternalFunction(funcName, funcInterface);
  }
  
  // Conditional = 'if' TestExpr TrueExpr 'else' FalseExpr
  Expression* parseConditional() {
    DEBUG_TRACE_PARSER;
    
    // Let the parent line level for test blocks be the column at which '?' is defined
    //LineLevel lineLevel = token_.column;
  
    // Conditional : Expression
    Conditional* conditional = new Conditional();

    nextToken(); // eat 'if'

    // Parse the test expression
    Expression* testExpression = parseExpression();
    if (testExpression == 0) return 0;
    conditional->setTestExpression(testExpression);

    // Parse the true branch
    Block* block = parseBlock();
    if (block == 0) return 0;
    conditional->setTrueBlock(block);

    // Expect 'else'
    if (token_.type != Token::Else) {
      return error("Premature end of conditional (expected 'else')");
    }
    nextToken(); // eat 'else'

    // Parse the false branch
    block = parseBlock();
    if (block == 0) return 0;
    conditional->setFalseBlock(block);

    #if DEBUG_PARSER
    rlog("Parsed conditional: " << conditional->toString());
    #endif

    return conditional;
  }
  
  // RHS = Expression
  Expression *parseAssignmentRHS(Expression *lhs) {
    DEBUG_TRACE_PARSER;
    Expression *rhs = parseExpression();
    if (!rhs) return 0;
    return new BinaryOp('=', lhs, rhs, BinaryOp::SimpleLTR);
  }
  
  
  /// Expression =
  ///   ::= primary binop_rhs
  ///   ::= primary '='
  ///   ::= primary
  ///
  Expression *parseExpression() {
    DEBUG_TRACE_PARSER;
    Expression *lhs = parsePrimary();
    if (!lhs) return 0;
    
    //// Backslash means "ignore the following sequence of linebreaks and comments
    ////                  and treat whatever is after it as the same line"
    //if (token_.type == Token::Backslash) {
    //  // Eat <comment>*<LF><comment>* and continue
    //  while (nextToken(/* newLineUpdatesLineLevel = */false).type == Token::NewLine
    //                                                     || token_.type == Token::Comment ) {}
    //}
    
    if (token_.type == Token::BinaryOperator || token_.type == Token::BinaryComparisonOperator) {
      // LHS binop RHS
      return parseBinOpRHS(0, lhs);

    } else if (token_.type == Token::BinaryOperator || token_.type == Token::BinaryComparisonOperator) {
      // LHS binop RHS
      return parseBinOpRHS(0, lhs);

    } else if (token_.type == Token::Assignment) {
      // LHS = RHS
      //nextToken(); // eat '='
      Expression *assignment = parseAssignmentRHS(lhs);
      if (!assignment) delete lhs;
      return assignment;

    } else if (token_.type == Token::Unexpected) {
      error("Unexpected token when expecting a left-hand-side expression");
      nextToken(); // Skip token for error recovery.
      delete lhs;
      return 0;

    } else {
      return lhs;
    }
  }
  
  // ParenExpression = '(' Expression ')'
  Expression *parseParen() {
    DEBUG_TRACE_PARSER;
    nextToken(); // eat '('
    
    ScopeFlag<bool> sf0(&isParsingCallArguments_, false);
    
    Expression* expression = parseExpression();
    if (expression == 0) return 0;
    
    // Expect terminating ')'
    if (token_.type != Token::RightParen) {
      return error("Unexpected token when expecting ')'");
    }
    nextToken(); // eat ')'
    
    return expression;
  }
  
  
  // ListLiteral = '[' ListItem? (SP+ ListItem)* ']'
  // ListItem = Expression
  // -- All ListItems need to be of the same type
  Expression *parseListLiteral() {
    DEBUG_TRACE_PARSER;
    nextToken(); // eat '['
    
    ListLiteral* listLit = new ListLiteral();
    
    ScopeFlag<bool> sf0(&isParsingCallArguments_, false);
    
    while (token_.type != Token::RightSqBracket) {
      Expression* expression = parseExpression();
      rlog("Parsed expression " << expression->toString());
      if (expression == 0) return 0;
      listLit->addExpression(expression);
    }
    
    // Expect terminating ']'
    if (token_.type != Token::RightSqBracket) {
      return error("Unexpected token when expecting ']'");
    }
    nextToken(); // eat ']'
    
    return listLit;
  }
  
  
  // IntLiteral = '0' | [1-9][0-9]*
  Expression *parseIntLiteral() {
    DEBUG_TRACE_PARSER;
    Expression *expression = new IntLiteral(token_.textValue, token_.intValue);
    nextToken(); // consume
    return expression;
  }
  
  // FloatLiteral = [0-9] '.' [0-9]*
  Expression *parseFloatLiteral() {
    DEBUG_TRACE_PARSER;
    Expression *expression = new FloatLiteral(token_.textValue);
    nextToken(); // consume
    return expression;
  }
  
  // BoolLiteral = 'true' | 'false'
  Expression *parseBoolLiteral() {
    DEBUG_TRACE_PARSER;
    Expression *expression = new BoolLiteral(static_cast<bool>(token_.intValue));
    nextToken(); // consume
    return expression;
  }
  
  // DataLiteral = ''' <any octet excluding ''' unless after '\'>* '''
  Expression *parseDataLiteral() {
    DEBUG_TRACE_PARSER;
    Expression *expression = new DataLiteral(token_.textValue.rawByteString());
    nextToken(); // consume
    return expression;
  }
  
  // TextLiteral = '"' <any octet excluding '"' unless after '\'>* '"'
  Expression *parseTextLiteral() {
    DEBUG_TRACE_PARSER;
    Expression *expression = new TextLiteral(token_.textValue);
    nextToken(); // consume
    return expression;
  }
  
  // Primary = Literal | Identifier | IfExpr
  Expression* parsePrimary() {
    entry:
    DEBUG_TRACE_PARSER;
    switch (token_.type) {
      case Token::Identifier: {
        Expression* expr = parseIdentifierExpr();
        return expr;
      }
      case Token::IntLiteral:
        return parseIntLiteral();
      case Token::FloatLiteral:
        return parseFloatLiteral();
      case Token::BoolLiteral:
        return parseBoolLiteral();
      case Token::DataLiteral:
        return parseDataLiteral();
      case Token::TextLiteral:
        return parseTextLiteral();

      case Token::Func: {
        Function *func = parseFunction();
        #if DEBUG_PARSER
        std::cout << "Parsed function: " << (func ? func->toString() : std::string("<nil>")) << std::endl;
        #endif
        return func;
      }

      case Token::External:
        return parseExternalFunction();
      case Token::LeftParen:
        return parseParen();
      case Token::LeftSqBracket:
        return parseListLiteral();
      case Token::If:
        return parseConditional();

      case Token::NewLine: { // ignore
        nextToken();
        goto entry;
      }
      
      case Token::Error: return error(token_.textValue.UTF8String());
      case Token::End:   return error("Premature end");
      default:           return error("Unexpected token");
    }
  }
  
  // ------------------------------------------------------------------------
  
  inline const Token& _nextToken() {
    if (token_.isNull()) {
      token_ = const_cast<Token&>(tokens_.next());
      futureToken_ = const_cast<Token&>(tokens_.next());
    } else {
      previousToken_ = token_;
      token_ = futureToken_;
      futureToken_ = const_cast<Token&>(tokens_.next());
    }
    return token_;
  }
  
  // nextToken() -- Advances the token stream one token forward and returns
  //                the new token (a reference to the token_ instance variable).
  //
  // Affected instance variables (state after calling this function):
  //
  //   token_
  //     A copy of the token being logically read.
  //
  //   futureToken_
  //     A reference to the next token to be read.
  //
  //   currentLineLevel_
  //     Number of leading whitespace for the current line.
  //     The current line number can be aquired from token_.line
  //
  //   previousLineLevel_
  //     Number of leading whitespace for the previous line.
  //     The previous line number can be aquired from token_.line-1
  //
  const Token& nextToken() {
    _nextToken();
    
    recentComments_.clear();
    
    // Backslash means "ignore the following sequence of linebreaks and comments
    //                  and treat whatever is after it as the same line"
    if (token_.type == Token::Backslash) {
      // Eat <comment>*<LF><comment>*
      while (1) {
        _nextToken();
        if (token_.type == Token::Comment) {
          recentComments_.push_back(token_);
        } else if (token_.type != Token::NewLine) {
          break;
        }
      }
    } else {
      // Eat <comment>*
      while (token_.type == Token::Comment) {
        recentComments_.push_back(token_);
        _nextToken();
      }
    }
    
    // XXX DEBUG: Dump comments using rlog
    // if (recentComments_.size()) {
    //   std::vector<Token>::const_iterator it = recentComments_.begin();
    //   for (; it != recentComments_.end(); ++it) {
    //     rlog("recentComments_[" << "] = " << (*it).toString());
    //   }
    // }
    
    #if DEBUG_PARSER
    fprintf(stderr, "\e[34;1m>> %s\e[0m\n", token_.toString().c_str());
    #endif
    
    if (token_.type == Token::NewLine) {
      previousLineLevel_ = currentLineLevel_;
      currentLineLevel_ = token_.length;
    }
    return token_;
  }
  
  
  // parse() -- Parse a module. Returns the AST for the parsed code.
  Function *parseModule() {
    DEBUG_TRACE_PARSER;
    
    // Advance to first token in stream
    nextToken();
    
    // Parse the root block
    Block* block = parseBlock();
    
    // If we got a block, put it inside an anonymous func and return that func.
    if (block) {
      return new Function(new FunctionType(0, 0, /* isPublic = */ true), block);
    } else {
      return 0;
    }
  }
};

} // namespace hue

#endif // HUE__PARSER_H
