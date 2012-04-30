// Reads from a TokenBuffer and builds an AST, possibly producing warnings and errors
#ifndef RSMS_PARSER_H
#define RSMS_PARSER_H

#include "TokenBuffer.h"
#include "../Logger.h"
#include "../ast/Node.h"
#include "../ast/Block.h"
#include "../ast/Expression.h"
#include "../ast/Function.h"
#include "../ast/Conditional.h"

#include <vector>

#define DEBUG_PARSER 1
#if DEBUG_PARSER
  #include "../DebugTrace.h"
  #define DEBUG_TRACE_PARSER DEBUG_TRACE
#else
  #define DEBUG_TRACE_PARSER do{}while(0)
#endif


namespace rsms {
using namespace ast;


// Precedence of the pending binary operator token.
// This acts as a parse tree terminator -- when this returns -1, whatever expression
// is being parsed has reached its end and the parse branch ends/returns.
static int BinaryOperatorPrecedence(const Token& token) {
  // Modeled after JS op precedence, see:
  // https://developer.mozilla.org/en/JavaScript/Reference/Operators/Operator_Precedence
  if (token.type == Token::BinaryOperator) {
    switch (token.stringValue[0]) {
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
    switch (token.stringValue[0]) {
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
    , futureToken_(const_cast<Token&>(NullToken)) {}
  
  // ------------------------------------------------------------------------
  // Error handling
  Expression *error(const char *str) {
    std::ostringstream ss;
    ss << str << " (" << token_.toString() << ")";
    errors_.push_back(ss.str());
    
    #if DEBUG_PARSER
    fprintf(stderr, "\e[31;1mError: %s\e[0m (%s)\n Token trace:\n", str, token_.toString().c_str());
    // list token trace
    size_t n = 1; // excluding the futureToken_
    while (n < tokens_.count()) {
      if (tokens_[n].type == Token::NewLine && tokens_[n].line == 6) {
        std::cerr << "  \e[31;1m" << tokens_[n++].toString() << "\e[0m" << std::endl;
      } else
      std::cerr << "  " << tokens_[n++].toString() << std::endl;
    }
    #else
    fprintf(stderr, "\e[31;1mError: %s\e[0m (%s)\n", str, token_.toString().c_str());
    #endif
    
    return 0;
  }
  
  const std::vector<std::string>& errors() const { return errors_; };
  
  bool tokenTerminatesCall(const Token& token) const {
    return token.type != Token::Identifier
        && token.type != Token::IntLiteral
        && token.type != Token::FloatLiteral
        && token.type != Token::LeftParen
        && (token.type != Token::NewLine || currentLineLevel_ <= previousLineLevel_);
  }
  
  bool tokenIsType(const Token& token) const {
    return token.type == Token::Identifier
        || token.type == Token::IntSymbol
        || token.type == Token::FloatSymbol
        || token.type == Token::Bool
        || token.type == Token::Func;
  }
  
  // ------------------------------------------------------------------------
  
  // Variable = Identifier 'MUTABLE'? Type?
  Variable *parseVariable(std::string identifierName) {
    DEBUG_TRACE_PARSER;
    Type *T = NULL;
    bool isMutable = false;
    
    if (token_.type == Token::Mutable) {
      isMutable = true;
      nextToken(); // eat 'MUTABLE'
    }
    
    if (tokenIsType(token_)) {
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
  //   x 
  //
  VariableList *parseVariableList(std::string firstVarIdentifierName = std::string()) {
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
        std::string identifierName = token_.stringValue;
        nextToken(); // eat id
        variable = parseVariable(token_.stringValue);
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
  Expression *parseCall(std::string identifierName) {
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
    std::string identifierName = token_.stringValue;
    nextToken();  // eat identifier.
    
    if (token_.type == Token::Assignment || futureToken_.type == Token::Assignment) {
      // Look-ahead to solve the case: foo Bar = ... vs foo Bar baz (call)
      return parseAssignment(identifierName);
    } if (isParsingCallArguments_ || tokenTerminatesCall(token_)) {
       return new Symbol(identifierName);
    }
    
    return parseCall(identifierName);
  }
  
  
  // Assignment = VariableList '=' Expression
  Assignment *parseAssignment(std::string firstVarIdentifierName) {
    DEBUG_TRACE_PARSER;
    VariableList *varList = parseVariableList(firstVarIdentifierName);
    if (!varList) return 0;
    
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
    
    // LHS type inference
    // TODO: Support more than one return value
    if (   varList->size() == 1
        && (    !(*varList)[0]->type()
             || (*varList)[0]->type()->typeID() == Type::Unknown ) 
       )
    {
      if (rhs->isFunctionType()) {
        (*varList)[0]->setType(new Type(Type::Func));
      } else if (rhs->nodeTypeID() == Node::TIntLiteral) {
        (*varList)[0]->setType(new Type(Type::Int));
      } else if (rhs->nodeTypeID() == Node::TFloatLiteral) {
        (*varList)[0]->setType(new Type(Type::Float));
      }
    }
    
    return new Assignment(varList, rhs);
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
      char binOperator = token_.stringValue[0];
      BinaryOp::Type binType = BinaryOp::SimpleLTR;
      if (token_.type == Token::BinaryComparisonOperator) {
        binType = BinaryOp::EqualityLTR;
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
      lhs = new BinaryOp(binOperator, lhs, rhs, binType);
    }
  }
  
  // Type = 'Int' | 'Float' | 'func' | 'extern' | Identifier
  Type *parseType() {
    DEBUG_TRACE_PARSER;
    Type* T = 0;
         if (token_.type == Token::IntSymbol)   T = new Type(Type::Int);
    else if (token_.type == Token::FloatSymbol) T = new Type(Type::Float);
    else if (token_.type == Token::Func)        T = new Type(Type::Func);
    else if (token_.type == Token::Bool)        T = new Type(Type::Bool);
    else if (token_.type == Token::Identifier)  T = new Type(token_.stringValue);
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
  
  // Entry points:
  //   FunctionExpr   = "func" FunctionType
  //   ExternalExpr   = "external" Identifier FunctionType
  //
  // FunctionType     = Parameters? Result?
  //   Result         = TypeList
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
  FunctionType *parseFunctionType() {
    DEBUG_TRACE_PARSER;
    
    VariableList *variableList = NULL;
    TypeList *returnTypes = NULL;
  
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
    if (tokenIsType(token_)) {
      // TypeList =
      returnTypes = parseTypeList();
      if (!returnTypes) {
        if (variableList) delete variableList;
        return 0;
      }
    }
  
    // Create function interface
    return new FunctionType(variableList, returnTypes);
  }
  
  
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
  Block* parseBlock(LineLevel outerLineLevel) {
    DEBUG_TRACE_PARSER;
    Block* block = new Block();
    bool notARootBlock = outerLineLevel != RootLineLevel;
    
    while (token_.type != Token::End) {
      
      // If we should infer the line level, take the line level from the line we just parsed
      if (outerLineLevel == InferLineLevel) {
        outerLineLevel = currentLineLevel_;
        rlog("Inferred outerLineLevel to " << outerLineLevel);
      }
      
      // Read one expression
      Expression *expr = parseExpression();
      if (expr == 0) {
        delete block; // TODO: cleanup
        return 0;
      }
      
      // Add the expression to the function body
      block->addNode(expr);
      
      // Read any linebreaks and check the line level after each linebreak, starting with the one
      // we just read.
      while (token_.type == Token::NewLine) {
        if (notARootBlock && currentLineLevel_ <= outerLineLevel) {
          #if DEBUG_PARSER
          rlog("Block ended (line level drop)");
          #endif
          nextToken(); // eat NewLine
          goto after_outer_loop;
        }
        nextToken();
      }
      
      // Semicolon terminates a non-root block (and is illegal in a root block)
      if (token_.type == Token::Semicolon) {
        if (notARootBlock) {
          nextToken(); // Eat the semicolon
          #if DEBUG_PARSER
          rlog("Block ended (semicolon)");
          #endif
          break; // terminate block
        } else {
          error("Unexpected semicolon token while parsing non-root block");
          delete block; // TODO: cleanup
          return 0;
        }
      }
      
      // Woops, unexpected token?
      if (token_.type == Token::Unexpected) {
        nextToken(); // skip for error recovery
        error("Unexpected token while parsing block");
        delete block; // TODO: cleanup
        return 0;
      }
    }
    after_outer_loop:
    
    return block;
  }
  
  
  // Function = 'func' FunctionType ':' BlockExpression
  Function *parseFunction() {
    DEBUG_TRACE_PARSER;
    nextToken();  // eat 'func'
    
    LineLevel funcLineLevel = currentLineLevel_;
    
    // Parse function interface
    FunctionType *interface = parseFunctionType();
    if (interface == 0) return 0;
    
    // Require ':'
    if (token_.type != Token::Colon) {
      delete interface;
      return (Function*)error("Expected ':' after function interface");
    }
    nextToken();  // eat ':'
    
    // Parse body
    Block* body = parseBlock(funcLineLevel);
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
    std::string funcName = token_.stringValue;
    nextToken(); // eat id
    
    FunctionType *funcInterface = parseFunctionType();
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
  
  // IfTestExpr = Expression ':'
  Expression* parseIfTestExpr() {
    DEBUG_TRACE_PARSER;
    
    // Parse the expression to be tested for truth
    Expression *testExpr = parseExpression();
    
    // expect ':' and return
    if (testExpr != 0 && token_.type != Token::Colon)
      return error("Expected colon after logical '?' test");
    nextToken(); // eat ':' (or try to recover from error)
    
    rlog("TEST " << (testExpr ? testExpr->toString() : "<nil>"));
    
    return testExpr;
  }
  
  // IfExpr = (IfTestExpr BlockExpression)+ BlockExpression
  Expression* parseIfExpr() {
    DEBUG_TRACE_PARSER;
    
    // Let the parent line level for test blocks be the column at which '?' is defined
    LineLevel lineLevel = token_.column;
    //LineLevel lineLevel = currentLineLevel_;
    //LineLevel lineLevel = InferLineLevel;
    
    nextToken(); // eat 'if'
  
    // Conditional : Expression
    Conditional* conditionals = new Conditional();
    
    // Treat this scope as parsing arguments -- this means that precedence changes so
    // we can do "if A B else C" --> (ifexpr ((if A) B), (else C)). Oterwise we would get:
    // "if A B else C" --> (ifexpr ((if (A B else C))
    //{ ScopeFlag<bool> sf0(&isParsingCallArguments_, true);
    
      while (1) {
        // Parse the test expression
        Expression* testExpr = parseExpression();
        if (testExpr == 0) return 0;
    
        // Parse the block expression to be the branch of the test expression
        Block* block = parseBlock(lineLevel);
        if (block == 0) return 0;
      
        // Add to conditionals
        //rlog("IF " << testExpr->toString() << " THEN " << block->toString());
        conditionals->addConditionalBranch(testExpr, block);
      
        // Expect 'if' | 'else'
        if (token_.type == Token::If) {
          // Another test in the chain
          nextToken(); // Eat 'if'
        } else if (token_.type == Token::Else) {
          // Fallback branch in the chain -- break
          break;
        } else {
          nextToken(); // Attempt error recovery
          return error("Expected 'if' or 'else'");
        }
      }
    // } // end of ScopeFlag<bool> sf0(&isParsingCallArguments_, true);
    
    // Parse the default branch
    assert(token_.type == Token::Else);
    nextToken(); // Eat 'else'
    Block* block = parseBlock(lineLevel);
    if (block == 0) return 0;
    conditionals->setDefaultBlock(block);
    
    //rlog("Parsed conditionals: " << conditionals->toString());
    return conditionals;
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
  
  // IntLiteral = '0' | [1-9][0-9]*
  Expression *parseIntLiteral() {
    DEBUG_TRACE_PARSER;
    Expression *expression = new IntLiteral(token_.stringValue, token_.intValue);
    nextToken(); // consume
    return expression;
  }
  
  // FloatLiteral = [0-9] '.' [0-9]*
  Expression *parseFloatLiteral() {
    DEBUG_TRACE_PARSER;
    Expression *expression = new FloatLiteral(token_.stringValue);
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
      
      case Token::If:
        return parseIfExpr();

      case Token::NewLine: { // ignore
        nextToken();
        goto entry;
      }
      default: return error("Unexpected token");
    }
  }
  
  // ------------------------------------------------------------------------
  
  inline const Token& _nextToken() {
    if (token_.isNull()) {
      token_ = const_cast<Token&>(tokens_.next());
      futureToken_ = const_cast<Token&>(tokens_.next());
    } else {
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
    Block* block = parseBlock(RootLineLevel);
    
    // If we got a block, put it inside an anonymous func and return that func.
    if (block) {
      return new Function(new FunctionType(0, 0, /* isPublic = */ true), block);
    } else {
      return 0;
    }
  }
};

} // namespace rsms

#endif // RSMS_PARSER_H
