// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

// Expressions
#ifndef HUE__AST_EXPRESSION_H
#define HUE__AST_EXPRESSION_H

#include "Node.h"
#include "Type.h"
#include "Variable.h"
#include "../Logger.h"
#include "../Text.h"

#include <vector>

namespace hue { namespace ast {

class FunctionType;

class Expression;
typedef std::vector<Expression*> ExpressionList;

// Base class for all expression nodes.
class Expression : public Node {
public:
  Expression(NodeTypeID t, const Type* resultType) : Node(t), resultType_(resultType) {}
  Expression(NodeTypeID t = TExpression, Type::TypeID resultTypeID = Type::Unknown)
      : Node(t), resultType_(new Type(resultTypeID)) {}
  virtual ~Expression() {}

  // Type of result from this expression
  virtual const Type *resultType() const { return resultType_; }
  virtual void setResultType(const Type* T) {
    resultType_ = T;
  }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "(?)";
    return ss.str();
  }

protected:
  const Type* resultType_;
};

// Numeric integer literals like "3".
class IntLiteral : public Expression {
  Text value_;
  const uint8_t radix_;
public:
  IntLiteral(Text value, uint8_t radix = 10)
    : Expression(TIntLiteral, Type::Int), value_(value), radix_(radix) {}

  const Text& text() const { return value_; }
  const uint8_t& radix() const { return radix_; }
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << value_;
    return ss.str();
  }
};

// Numeric fractional literals like "1.2".
class FloatLiteral : public Expression {
  Text value_;
public:
  FloatLiteral(Text value) : Expression(TFloatLiteral, Type::Float), value_(value) {}
  const Text& text() const { return value_; }
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << value_;
    return ss.str();
  }
};

// Boolean literal, namely "true" or "false"
class BoolLiteral : public Expression {
  bool value_;
public:
  BoolLiteral(bool value) : Expression(TBoolLiteral, Type::Bool), value_(value) {}
  inline bool isTrue() const { return value_; }
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << (value_ ? "true" : "false");
    return ss.str();
  }
};

/// Assigning a value to a symbol or variable, e.g. foo = 5
class Assignment : public Expression {
public:
  
  Assignment(Variable *var, Expression *rhs)
    : Expression(TAssignment), var_(var), rhs_(rhs) {}

  const Variable* variable() const { return var_; };
  Expression* rhs() const { return rhs_; }

  virtual const Type *resultType() const {
    if (rhs_ && var_->hasUnknownType()) {
      // rloge("CASE 1 ret " << rhs_->resultType()->toString()
      //       << " from " << rhs_->toString() << " (" << rhs_->typeName() << ")");
      return rhs_->resultType();
    } else {
      //rloge("CASE 2 ret " << var_->type()->toString());
      return var_->type();
    }
  }

  virtual void setResultType(const Type* T) throw(std::logic_error) {
    throw std::logic_error("Can not set type for compound 'Assignment' expression");
  }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    //NodeToStringHeader(level, ss);
    ss << "(= "
       << var_->toString(level+1)
       << " "
       << rhs_->toString(level+1)
       << ')';
    return ss.str();
  }
private:
  Variable* var_;
  Expression* rhs_;
};


}} // namespace hue::ast
#endif // HUE__AST_EXPRESSION_H
