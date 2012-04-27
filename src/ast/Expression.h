// Expressions
#ifndef RSMS_AST_EXPRESSION_H
#define RSMS_AST_EXPRESSION_H
#include "Node.h"
#include "TypeDeclaration.h"
#include "Variable.h"
#include <vector>

namespace rsms { namespace ast {

// Base class for all expression nodes.
class Expression : public Node {
public:
  Expression(Type t = TExpression) : Node(t) {}
  virtual ~Expression() {}
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Expression>";
    return ss.str();
  }
};

// Numeric integer literals like "3".
class IntLiteralExpression : public Expression {
  std::string value_;
  const uint8_t radix_;
public:
  IntLiteralExpression(std::string value, uint8_t radix = 10)
    : Expression(TIntLiteralExpression), value_(value), radix_(radix) {}

  const std::string& text() const { return value_; }
  const uint8_t& radix() const { return radix_; }
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<IntLiteralExpression value=" << value_ << '>';
    return ss.str();
  }
};

// Numeric fractional literals like "1.2".
class FloatLiteralExpression : public Expression {
  std::string value_;
public:
  FloatLiteralExpression(std::string value) : Expression(TFloatLiteralExpression), value_(value) {}
  const std::string& text() const { return value_; }
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<FloatLiteralExpression value=" << value_ << '>';
    return ss.str();
  }
};

// Referencing a symbol, like "a".
class SymbolExpression : public Expression {
  std::string name_;
public:
  SymbolExpression(const std::string &name) : Expression(TSymbolExpression), name_(name) {}
  const std::string& name() const { return name_; }
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<SymbolExpression name=" << name_ << '>';
    return ss.str();
  }
};

/// Assigning a value to a symbol or variable, e.g. foo = 5
class AssignmentExpression : public Expression {
public:
  
  AssignmentExpression(VariableList *varList, Expression *rhs)
    : Expression(TAssignmentExpression), varList_(varList), rhs_(rhs) {}

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
    ss << "<AssignmentExpression ";
    
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
class BinaryExpression : public Expression {
  char operator_;
  Expression *lhs_, *rhs_;
public:
  BinaryExpression(char op, Expression *lhs, Expression *rhs)
      : Expression(TBinaryExpression), operator_(op) , lhs_(lhs) , rhs_(rhs) {}
  const char& operatorValue() const { return operator_; }
  Expression *lhs() const { return lhs_; }
  Expression *rhs() const { return rhs_; }
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<BinaryExpression '" << operator_ << "' ("
       << (lhs_ ? lhs_->toString(level+1) : "<null>")
       << ", "
       << (rhs_ ? rhs_->toString(level+1) : "<null>")
       << ")>";
    return ss.str();
  }
};

// Function calls.
class CallExpression : public Expression {
  std::string callee_;
  std::vector<Expression*> args_;
public:
  CallExpression(const std::string &callee, std::vector<Expression*> &args)
    : Expression(TCallExpression), callee_(callee), args_(args) {}

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<CallExpression callee='" << callee_ << "' args=(";
    std::vector<Expression*>::const_iterator it;
    if ((it = args_.begin()) < args_.end()) { ss << (*it)->toString(level+1); it++; }
    for (; it < args_.end(); it++) {          ss << ", " << (*it)->toString(level+1); }
    return ss.str();
  }
};


}} // namespace rsms::ast
#endif // RSMS_AST_EXPRESSION_H
