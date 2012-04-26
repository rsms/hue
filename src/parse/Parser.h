// Reads from a TokenBuffer and builds an AST, possibly producing warnings and errors
// clang++ -std=c++11 -o parse src/parse2.cc && ./parse
#ifndef RSMS_PARSER_H
#define RSMS_PARSER_H

#include "TokenBuffer.h"
#include "../ast/Node.h"
#include "../ast/Block.h"
#include "../ast/Expression.h"
#include "../ast/Function.h"

#include <vector>

namespace rsms {
using namespace ast;

// ------------------------------------------------------------------
class RTracer {
  const char *funcName_;
  const char *funcInterface_;
public:
  static int depth;
  RTracer(const char *funcName, const char *funcInterface) {
    funcName_ = funcName;
    funcInterface_ = funcInterface;
    fprintf(stderr, "%*s\e[33;1m-> %d %s  \e[30;1m%s\e[0m\n", RTracer::depth*2, "",
            RTracer::depth, funcName_, funcInterface_);
    ++RTracer::depth;
  }
  ~RTracer() {
    --RTracer::depth;
    fprintf(stderr, "%*s\e[33m<- %d %s  \e[30;1m%s\e[0m\n", RTracer::depth*2, "",
            RTracer::depth, funcName_, funcInterface_);
  }
};
int RTracer::depth = 0;
#define R_TRACE RTracer RTracer_##__LINE__(__FUNCTION__, __PRETTY_FUNCTION__)
// ------------------------------------------------------------------


// Precedence of the pending binary operator token.
static int BinaryOperatorPrecedence(const Token& token) {
  if (token.type != Token::BinaryOperator)
    return -1;
  switch (token.stringValue[0]) {
    case '*': return 40;
    case '-': return 20;
    case '+': return 20;
    case '<': return 10;
    default: return -1;
  }
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
    
    fprintf(stderr, "\e[31;1mError: %s\e[0m (%s)\n Token trace:\n", str, token_.toString().c_str());
    
    // list token trace
    size_t n = 1; // excluding the futureToken_
    while (n < tokens_.count()) {
      if (tokens_[n].type == Token::NewLine && tokens_[n].line == 6) {
        std::cerr << "  \e[31;1m" << tokens_[n++].toString() << "\e[0m" << std::endl;
      } else
      std::cerr << "  " << tokens_[n++].toString() << std::endl;
    }
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
  
  bool tokenIsTypeDeclaration(const Token& token) const {
    return token.type == Token::Identifier
        || token.type == Token::IntSymbol
        || token.type == Token::FloatSymbol
        || token.type == Token::Func;
  }
  
  // ------------------------------------------------------------------------
  
  /// typedecl ::= id
  TypeDeclaration *parseTypeDeclaration() {
    R_TRACE;
    if (token_.type == Token::IntSymbol) {
      nextToken();
      return new TypeDeclaration(TypeDeclaration::Int);
    } else if (token_.type == Token::FloatSymbol) {
      nextToken();
      return new TypeDeclaration(TypeDeclaration::Float);
    } else if (token_.type == Token::Func) {
      nextToken();
      return new TypeDeclaration(TypeDeclaration::Func);
    } else if (token_.type == Token::Identifier) {
      std::string identifierName = token_.stringValue;
      nextToken();
      return new TypeDeclaration(identifierName);
    } else {
      error("Unexpected token while expecting type identifier");
      nextToken(); // Skip token for error recovery.
      return 0;
    }
  }
  
  /// var
  ///   ::= id 'MUTABLE'? typedecl?
  Variable *parseVariable(std::string identifierName) {
    R_TRACE;
    TypeDeclaration *typeDeclaration = NULL;
    bool isMutable = false;
    
    if (token_.type == Token::Mutable) {
      isMutable = true;
      nextToken(); // eat 'MUTABLE'
    }
    
    if (tokenIsTypeDeclaration(token_)) {
      typeDeclaration = parseTypeDeclaration();
      if (!typeDeclaration) return 0;
    }
    
    return new Variable(isMutable, identifierName, typeDeclaration);
  }
  
  /// varlist
  ///   ::= (var ',')* var
  VariableList *parseVariableList(std::string firstVarIdentifierName = std::string()) {
    R_TRACE;
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
  
  
  /// assignment_expr
  ///   ::= varlist '=' expr
  AssignmentExpression *parseAssignmentExpression(std::string firstVarIdentifierName) {
    R_TRACE;
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
             || (*varList)[0]->type()->type == TypeDeclaration::Unknown ) 
       )
    {
      if (rhs->type == Node::TFunction || rhs->type == Node::TExternalFunction) {
        (*varList)[0]->setType(new TypeDeclaration(TypeDeclaration::Func));
      } else if (rhs->type == Node::TIntLiteralExpression) {
        (*varList)[0]->setType(new TypeDeclaration(TypeDeclaration::Int));
      } else if (rhs->type == Node::TFloatLiteralExpression) {
        (*varList)[0]->setType(new TypeDeclaration(TypeDeclaration::Float));
      }
    }
    
    return new AssignmentExpression(varList, rhs);
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
  Expression *parseCallExpression(std::string identifierName) {
    R_TRACE;
    
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
    
    //printf("CallExpression to '%s' w/ %lu args\n", identifierName.c_str(), args.size());
    return new CallExpression(identifierName, args);
  }
  
  
  /// identifierexpr
  ///   ::= identifier
  ///   ::= identifier '(' expression* ')'
  Expression *parseIdentifierExpr() {
    R_TRACE;
    std::string identifierName = token_.stringValue;
    nextToken();  // eat identifier.
    
    if (token_.type == Token::Assignment || futureToken_.type == Token::Assignment) {
      // Look-ahead to solve the case: foo Bar = ... vs foo Bar baz (call)
      return parseAssignmentExpression(identifierName);
    } if (isParsingCallArguments_ || tokenTerminatesCall(token_)) {
       return new SymbolExpression(identifierName);
    }
    
    return parseCallExpression(identifierName);
  }
  
  
  /// binop_rhs
  ///   ::= (op primary)*
  Expression *parseBinOpRHS(int lhsPrecedence, Expression *lhs) {
    R_TRACE;
    // If this is a binop, find its precedence.
    while (1) {
      int precedence = BinaryOperatorPrecedence(token_);
    
      // If this is a binop that binds at least as tightly as the current binop,
      // consume it, otherwise we are done.
      if (precedence < lhsPrecedence)
        return lhs;
    
      // Okay, we know this is a binop.
      char binOperator = token_.stringValue[0];
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
      lhs = new BinaryExpression(binOperator, lhs, rhs);
    }
  }
  
  /// typelist
  ///   ::= (typedecl ',')* typedecl
  TypeDeclarationList *parseTypeDeclarations() {
    TypeDeclarationList *typeList = new TypeDeclarationList();
    
    while (1) {
      TypeDeclaration *typeDeclaration = parseTypeDeclaration();
      if (!typeDeclaration) {
        delete typeList; // todo: delete contents
        return 0;
      }
      
      typeList->push_back(typeDeclaration);
      
      // Comma means there are more types
      if (token_.type != Token::Comma) break;
      nextToken(); // eat ','
      
    }
    
    return typeList;
  }
  
    
  /// func_interface
  ///   ::= arglist? typelist?
  ///   arglist  ::= '(' varlist ')'
  //
  // Examples:
  //   func ->
  //   func (Number x) ->
  //   func (Number x) Number ->
  //   func Number ->
  //   func (Number x, Number y) Number ->
  //   func (Number x, Number y) Number, Number ->
  //   extern atan (Number x, Number y) Number linebreak
  //
  FunctionInterface *parseFunctionInterface() {
    R_TRACE;
    
    VariableList *variableList = NULL;
    TypeDeclarationList *returnTypes = NULL;
  
    /// arglist?
    if (token_.type == Token::LeftParen) {
      /// arglist  ::= '(' varlist ')'
      nextToken(); // eat '('
      
      variableList = parseVariableList();
      if (!variableList) return 0;
    
      // Require argument terminator: ')'
      if (token_.type != Token::RightParen) {
        return (FunctionInterface*)error("Expected ')' in function definition");
      }
      nextToken();  // eat ')'.
    }
    
    // return type declarations?
    if (tokenIsTypeDeclaration(token_)) {
      returnTypes = parseTypeDeclarations();
      if (!returnTypes) {
        if (variableList) delete variableList;
        return 0;
      }
    }
  
    // Create function interface
    return new FunctionInterface(variableList, returnTypes);
  }
  
  
  /// func_definition
  ///   ::= 'func' func_interface '->' expression
  Function *parseFunction() {
    R_TRACE;
    nextToken();  // eat 'func'
    
    // Parse function interface
    FunctionInterface *interface = parseFunctionInterface();
    if (interface == 0) return 0;
    
    // Require '->'
    if (token_.type != Token::RightArrow) {
      delete interface;
      return (Function*)error("Expected '->' after function interface");
    }
    nextToken();  // eat '->'

    // Parse function body
    Expression *body = parseExpression();
    if (body == 0) {
      delete interface;
      return 0;
    }
    
    return new Function(interface, body);
  }
  

  /// external ::= 'extern' id func_interface linebreak
  ExternalFunction *parseExternalFunction() {
    R_TRACE;
    nextToken();  // eat 'extern'
    
    if (token_.type != Token::Identifier) {
      return (ExternalFunction*)error("Expected name after 'extern'");
    }
    
    // Remember id
    std::string funcName = token_.stringValue;
    nextToken(); // eat id
    
    FunctionInterface *funcInterface = parseFunctionInterface();
    if (!funcInterface) return 0;
    
    // Require terminating linebreak
    if (token_.type != Token::NewLine) {
      delete funcInterface;
      return (ExternalFunction*)error("Expected linebreak after external declaration");
    }
    nextToken(); // eat linebreak
    
    return new ExternalFunction(funcName, funcInterface);
  }
  

  /// toplevel ::= expression
  Function *parseTopLevelExpression() {
    R_TRACE;
    Expression *body = parseExpression();
    if (body == 0) return 0;
    // anonymous interface
    FunctionInterface *interface = new FunctionInterface();
    return new Function(interface, body);
  }
  
  
  /// assignment
  ///   ::= expression = expression
  Expression *parseAssignmentRHS(Expression *lhs) {
    R_TRACE;
    Expression *rhs = parseExpression();
    if (!rhs) return 0;
    return new BinaryExpression('=', lhs, rhs);
  }
  
  
  /// expression
  ///   ::= primary binop_rhs
  ///   ::= primary '='
  ///   ::= primary
  ///
  Expression *parseExpression() {
    R_TRACE;
    Expression *lhs = parsePrimary();
    if (!lhs) return 0;
    
    if (token_.type == Token::BinaryOperator) {
      // LHS binop RHS
      return parseBinOpRHS(0, lhs);
    } else if (token_.type == Token::Assignment) {
      // LHS = RHS
      nextToken(); // eat '='
      Expression *assignment = parseAssignmentRHS(lhs);
      if (!assignment) delete lhs;
      return assignment;
    } else if (token_.type == Token::Unexpected) {
      error("Unexpected token when expecting a left-hand side expression");
      nextToken(); // Skip token for error recovery.
      delete lhs;
      return 0;
    } else {
      // LHS
      return lhs;
    }
  }
  
  /// intliteral ::= '0' | [1-9][0-9]*
  Expression *parseIntLiteralExpr() {
    R_TRACE;
    Expression *expression = new IntLiteralExpression(token_.longValue);
    nextToken(); // consume the number
    return expression;
  }
  
  /// loatliteral ::= [0-9] '.' [0-9]*
  Expression *parseFloatLiteralExpr() {
    R_TRACE;
    Expression *expression = new FloatLiteralExpression(token_.doubleValue);
    nextToken(); // consume the number
    return expression;
  }
  
  /// primary
  ///   ::= identifier
  ///   ::= number
  ///   ::= paren
  Expression *parsePrimary() {
    entry:
    R_TRACE;
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
        std::cout << "Parsed function: " << func->toString() << std::endl;
        return func;
      }

      case Token::External:
        return parseExternalFunction();

      //case '(':            return ParseParenExpr();
      case Token::Comment:
      case Token::NewLine: { // ignore
        nextToken();
        goto entry;
      }
      default: return error("Unexpected token when expecting an expression");
    }
  }
  
  // ------------------------------------------------------------------------
  
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
    if (token_.isNull()) {
      token_ = const_cast<Token&>(tokens_.next());
      futureToken_ = const_cast<Token&>(tokens_.next());
    } else {
      token_ = futureToken_;
      futureToken_ = const_cast<Token&>(tokens_.next());
    }
    fprintf(stderr, "\e[34;1m>> %s\e[0m\n", token_.toString().c_str());
    if (token_.type == Token::NewLine) {
      previousLineIndentation_ = currentLineIndentation_;
      currentLineIndentation_ = token_.length;
    }
    return token_;
  }
  
  // parse() -- Parse a module. Returns the AST for the parsed code.
  Function *parse() {
    Block *moduleBlock = NULL;
    
    while (1) {
      switch (nextToken().type) {
        
        case Token::End:
          goto done_parsing;
        
        case Token::Comment:
        case Token::NewLine:
          break; // ignore
        
        case Token::Identifier: {
          Expression *expr = parseExpression();
          if (expr) {
            std::cout << "Parsed module expression: " << expr->toString() << std::endl;
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
      FunctionInterface *moduleFuncInterface = new FunctionInterface();
      Function *moduleFunc = new Function(moduleFuncInterface, moduleBlock);
      std::cout << "Parsed module: " << moduleBlock->toString() << std::endl;
      return moduleFunc;
    }
    
    return NULL;
  }
};

} // namespace rsms

#endif // RSMS_PARSER_H
