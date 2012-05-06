// Expressions
#ifndef RSMS_AST_EXPRESSION_H
#define RSMS_AST_EXPRESSION_H

#include "Node.h"
#include "Type.h"
#include "Variable.h"

#include "../Text.h"

#include <vector>

namespace rsms { namespace ast {

// Base class for all expression nodes.
class Expression : public Node {
public:
  Expression(NodeTypeID t = TExpression) : Node(t) {}
  virtual ~Expression() {}
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<Expression>";
    return ss.str();
  }
};

// Numeric integer literals like "3".
class IntLiteral : public Expression {
  Text value_;
  const uint8_t radix_;
public:
  IntLiteral(Text value, uint8_t radix = 10)
    : Expression(TIntLiteral), value_(value), radix_(radix) {}

  const Text& text() const { return value_; }
  const uint8_t& radix() const { return radix_; }
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<IntLiteral value=" << value_ << '>';
    return ss.str();
  }
};

// Numeric fractional literals like "1.2".
class FloatLiteral : public Expression {
  Text value_;
public:
  FloatLiteral(Text value) : Expression(TFloatLiteral), value_(value) {}
  const Text& text() const { return value_; }
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<FloatLiteral value=" << value_ << '>';
    return ss.str();
  }
};

// Boolean literal, namely "true" or "false"
class BoolLiteral : public Expression {
  bool value_;
public:
  BoolLiteral(bool value) : Expression(TBoolLiteral), value_(value) {}
  inline bool isTrue() const { return value_; }
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<BoolLiteral " << (value_ ? "true" : "false") << '>';
    return ss.str();
  }
};

// Referencing a symbol, like "a".
class Symbol : public Expression {
  Text name_;
public:
  Symbol(const Text &name) : Expression(TSymbol), name_(name) {}
  const Text& name() const { return name_; }
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<Symbol name=" << name_ << '>';
    return ss.str();
  }
};

/// Assigning a value to a symbol or variable, e.g. foo = 5
class Assignment : public Expression {
public:
  
  Assignment(VariableList *varList, Expression *rhs)
    : Expression(TAssignment), varList_(varList), rhs_(rhs) {}

  const VariableList *variables() const { return varList_; };
  const Expression *rhs() const { return rhs_; }

  //VariableList *varList() { return varList_; }
  // 
  // void setRHS(Expression *rhs) {
  //   Expression *rhs2 = rhs_;
  //   rhs_ = rhs;
  //   if (rhs2) delete rhs2;
  // }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Assignment ";
    
    VariableList::const_iterator it;
    if ((it = varList_->begin()) < varList_->end()) { ss << (*it)->toString(level+1); it++; }
    for (; it < varList_->end(); it++) {          ss << ", " << (*it)->toString(level+1); }
    
    ss << " = " << rhs_->toString(level+1) << '>';
    return ss.str();
  }
private:
  VariableList *varList_;
  Expression *rhs_;
};

// Binary operators.
class BinaryOp : public Expression {
public:
  enum Type {
    SimpleLTR = 0, // '=', '+', '>', '&', etc -- operator_ holds the value
    EqualityLTR, // '<=' '>=' '!=' '==' -- operator_ holds the value of the first byte
  };
  
  BinaryOp(char op, Expression *lhs, Expression *rhs, Type type)
      : Expression(TBinaryOp), operator_(op) , lhs_(lhs) , rhs_(rhs), type_(type) {}
  
  inline Type type() const { return type_; }
  inline bool isEqualityLTRType() const { return type_ == EqualityLTR; }
  inline char operatorValue() const { return operator_; }
  Expression *lhs() const { return lhs_; }
  Expression *rhs() const { return rhs_; }
  
  std::string operatorName() const {
    if (type_ == EqualityLTR) {
      return std::string(1, '=') + operator_;
    } else {
      return std::string(1, operator_);
    }
  }
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<BinaryOp "
       << (lhs_ ? lhs_->toString(level+1) : "<null>")
       << " '" << operator_;
    if (type_ == EqualityLTR) ss << '=';
    ss << "' "
       << (rhs_ ? rhs_->toString(level+1) : "<null>")
       << ">";
    return ss.str();
  }
private:
  char operator_;
  Expression *lhs_, *rhs_;
  Type type_;
};

// Function calls.
class Call : public Expression {
public:
  typedef std::vector<Expression*> ArgumentList;
  
  Call(const Text &calleeName, ArgumentList &args)
    : Expression(TCall), calleeName_(calleeName), args_(args) {}

  const Text& calleeName() const { return calleeName_; }
  const ArgumentList& arguments() const { return args_; }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Call " << calleeName_ << " (";
    ArgumentList::const_iterator it;
    if ((it = args_.begin()) < args_.end()) { ss << (*it)->toString(level+1); it++; }
    for (; it < args_.end(); it++) {          ss << ", " << (*it)->toString(level+1); }
    ss << ")>";
    return ss.str();
  }
private:
  Text calleeName_;
  ArgumentList args_;
};


}} // namespace rsms::ast
#endif // RSMS_AST_EXPRESSION_H
