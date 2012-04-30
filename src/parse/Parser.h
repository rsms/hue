// Reads from a TokenBuffer and builds an AST, possibly producing warnings and errors
#ifndef RSMS_PARSER_H
#define RSMS_PARSER_H

#include "TokenBuffer.h"
#include "../Logger.h"
#include "../ast/Node.h"
#include "../ast/Block.h"
#include "../ast/Expression.h"
#include "../ast/Function.h"

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
  TokenBuffer& tokens_;
  Token token_;
  Token& futureToken_;
  bool isParsingCallArguments_ = false;
  uint32_t previousLineIndentation_ = 0;
  uint32_t currentLineIndentation_ = 0;
  
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
        && (token.type != Token::NewLine || currentLineIndentation_ <= previousLineIndentation_);
  }
  
  bool tokenIsType(const Token& token) const {
    return token.type == Token::Identifier
        || token.type == Token::IntSymbol
        || token.type == Token::FloatSymbol
        || token.type == Token::Func;
  }
  
  // ------------------------------------------------------------------------
  
  // Variable = Identifier 'MUTABLE'? Type?
  Variable *parseVariable(std::string identifierName) {
    DEBUG_TRACE_PARSER;
    Type *typeDeclaration = NULL;
    bool isMutable = false;
    
    if (token_.type == Token::Mutable) {
      isMutable = true;
      nextToken(); // eat 'MUTABLE'
    }
    
    if (tokenIsType(token_)) {
      typeDeclaration = parseType();
      if (!typeDeclaration) return 0;
    }
    
    return new Variable(isMutable, identifierName, typeDeclaration);
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
  
  
  // Assignment   = VariableList '=' Expression
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
      // TODO: Use parseType()
      if (rhs->isFunctionType()) {
        (*varList)[0]->setType(new Type(Type::Func));
        //if (rhs->functionType()->name().empty())
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
  
  // Type = 'Int' | 'Float' | 'func' | Identifier
  Type *parseType() {
    DEBUG_TRACE_PARSER;
    if (token_.type == Token::IntSymbol) {
      nextToken();
      return new Type(Type::Int);
    } else if (token_.type == Token::FloatSymbol) {
      nextToken();
      return new Type(Type::Float);
    } else if (token_.type == Token::Func) {
      nextToken();
      return new Type(Type::Func);
    } else if (token_.type == Token::Identifier) {
      std::string identifierName = token_.stringValue;
      nextToken();
      return new Type(identifierName);
    } else {
      error("Unexpected token while expecting type identifier");
      nextToken(); // Skip token for error recovery.
      return 0;
    }
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
      
      variableList = verifyAndUpdateVariableList(variableList);
      if (variableList == 0) return 0;
    
      // ')'
      if (token_.type != Token::RightParen) {
        return (FunctionType*)error("Expected ')' in function definition");
      }
      nextToken();  // eat ')'.
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
  
  
  /// func_definition
  ///   ::= 'func' func_interface '->' expression
  Function *parseFunction() {
    DEBUG_TRACE_PARSER;
    nextToken();  // eat 'func'
    
    // Parse function interface
    FunctionType *interface = parseFunctionType();
    if (interface == 0) return 0;
    
    // Require '->'
    if (token_.type != Token::RightArrow) {
      delete interface;
      return (Function*)error("Expected '->' after function interface");
    }
    nextToken();  // eat '->'
    
    // Is this potentially a multi-expression function body?
    uint32_t bodyStartedAtLineIndentation = UINT32_MAX;
    if (token_.type == Token::NewLine) {
      bodyStartedAtLineIndentation = currentLineIndentation_;
    }

    // Parse function body
    Block* body = new Block;
    
    while (1) {
      // Read one expression
      Expression *expr = parseExpression();
      if (expr == 0) {
        delete interface;
        return 0;
      }
      
      // Add the expression to the function body
      body->addNode(expr);
      
      // If we know this is a single-expression body, break after the first expression
      if (bodyStartedAtLineIndentation == UINT32_MAX) {
        #if DEBUG_PARSER
        rlog("Body ended (single-expression body)");
        #endif
        break;
      } else {
        
        // Body ends when we either get a non-newline (e.g. a terminating) token, or the
        // line indentation drops below the first line of the body
        if (token_.type != Token::NewLine || currentLineIndentation_ < bodyStartedAtLineIndentation) {
          #if DEBUG_PARSER
          rlog("Body ended (" << (token_.type != Token::NewLine ? "line indent drop" : "terminating token") << ")");
          #endif
          break;
        }
      }
    }
    
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
  
  
  /// assignment
  ///   ::= expression = expression
  Expression *parseAssignmentRHS(Expression *lhs) {
    DEBUG_TRACE_PARSER;
    Expression *rhs = parseExpression();
    if (!rhs) return 0;
    return new BinaryOp('=', lhs, rhs, BinaryOp::SimpleLTR);
  }
  
  
  /// expression
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
    //  while (nextToken(/* newLineUpdatesLineIndentation = */false).type == Token::NewLine
    //                                                     || token_.type == Token::Comment ) {}
    //}
    
    if (token_.type == Token::BinaryOperator || token_.type == Token::BinaryComparisonOperator) {
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
  
  /// intliteral ::= '0' | [1-9][0-9]*
  Expression *parseIntLiteralExpr() {
    DEBUG_TRACE_PARSER;
    Expression *expression = new IntLiteral(token_.stringValue, token_.intValue);
    nextToken(); // consume the number
    return expression;
  }
  
  /// loatliteral ::= [0-9] '.' [0-9]*
  Expression *parseFloatLiteralExpr() {
    DEBUG_TRACE_PARSER;
    Expression *expression = new FloatLiteral(token_.stringValue);
    nextToken(); // consume the number
    return expression;
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
  
  /// primary
  ///   ::= identifier
  ///   ::= number
  ///   ::= paren
  Expression *parsePrimary() {
    entry:
    DEBUG_TRACE_PARSER;
    switch (token_.type) {
      case Token::Identifier: {
        Expression* expr = parseIdentifierExpr();
        return expr;
      }
      case Token::IntLiteral:
        return parseIntLiteralExpr();
      case Token::FloatLiteral:
        return parseFloatLiteralExpr();

      case Token::Func: {
        Function *func = parseFunction();
        #if DEBUG_PARSER
        std::cout << "Parsed function: " << func->toString() << std::endl;
        #endif
        return func;
      }

      case Token::External:
        return parseExternalFunction();

      case Token::LeftParen:
        return parseParen();

      case Token::Comment:
      case Token::NewLine: { // ignore
        nextToken();
        goto entry;
      }
      default: return error("Unexpected token when expecting: id|intlit|floatlit|func|external|comment|newline");
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
  //   currentLineIndentation_
  //     Number of leading whitespace for the current line.
  //     The current line number can be aquired from token_.line
  //
  //   previousLineIndentation_
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
      previousLineIndentation_ = currentLineIndentation_;
      currentLineIndentation_ = token_.length;
    }
    return token_;
  }
  
  // parse() -- Parse a module. Returns the AST for the parsed code.
  Function *parse() {
    Block *moduleBlock = NULL;
    
    nextToken();
    
    while (1) {
      //switch (nextToken().type) {
      switch (token_.type) {
        
        case Token::End:
          goto done_parsing;
        
        case Token::Comment:
        case Token::NewLine:
          nextToken();
          break; // ignore
        
        case Token::Identifier: {
          Expression *expr = parseExpression();
          if (expr) {
            #if DEBUG_PARSER
            std::cout << "Parsed module expression: " << expr->toString() << std::endl;
            #endif
            
            if (moduleBlock == NULL) {
              moduleBlock = new Block(expr);
            } else {
              moduleBlock->addNode(expr);
            }
          } else {
            nextToken();  // Skip token for error recovery.
          }
          break;
        }
        
        //case Token::Unexpected:
        default: {
          error("Unexpected token at module level when expecting an expression");
          nextToken();  // Skip token for error recovery.
          break;
        }
      }
      
    }
    done_parsing:
    
    if (moduleBlock) {
      FunctionType *moduleFuncInterface = new FunctionType(0, 0, /* isPublic = */ true);
      Function *moduleFunc = new Function(moduleFuncInterface, moduleBlock);
      return moduleFunc;
    }
    
    return NULL;
  }
};

} // namespace rsms

#endif // RSMS_PARSER_H
