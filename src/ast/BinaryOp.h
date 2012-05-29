// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#ifndef HUE_AST_BINARYOP_H
#define HUE_AST_BINARYOP_H

#include "Expression.h"

namespace hue { namespace ast {

// Binary operators.
class BinaryOp : public Expression {
public:
  enum Kind {
    SimpleLTR = 0, // '=', '+', '>', '&', etc -- operator_ holds the value
    EqualityLTR, // '<=' '>=' '!=' '==' -- operator_ holds the value of the first byte
  };
  
  BinaryOp(char op, Expression *lhs, Expression *rhs, Kind kind)
      : Expression(TBinaryOp, Type::Bool), operator_(op) , lhs_(lhs) , rhs_(rhs), kind_(kind) {}
  
  inline Kind kind() const { return kind_; }
  inline bool isEqualityLTRKind() const { return kind_ == EqualityLTR; }
  inline bool isComparison() const {
    return isEqualityLTRKind() || operator_ == '<' || operator_ == '>';
  }
  inline char operatorValue() const { return operator_; }
  Expression *lhs() const { return lhs_; }
  Expression *rhs() const { return rhs_; }

  virtual Type *resultType() const {
    if (isComparison()) {
      return resultType_; // always Type::Bool
    } else if (!lhs_->resultType()->isUnknown()) {
      return lhs_->resultType();
    } else {
      return rhs_->resultType();
    }
  }

  virtual void setResultType(Type* T) {
    assert(T->isUnknown() == false); // makes no sense to set T=unknown
    // Call setResultType with T for LHS and RHS which are unknown
    if (lhs_ && lhs_->resultType()->isUnknown()) {
      lhs_->setResultType(T);
    }
    if (rhs_ && rhs_->resultType()->isUnknown()) {
      rhs_->setResultType(T);
    }
  }
  
  std::string operatorName() const {
    if (kind_ == EqualityLTR) {
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
    if (kind_ == EqualityLTR) ss << '=';
    ss << "' "
       << (rhs_ ? rhs_->toString(level+1) : "<null>")
       << ">";
    return ss.str();
  }
private:
  char operator_;
  Expression *lhs_, *rhs_;
  Kind kind_;
};


}} // namespace hue::ast
#endif // HUE_AST_BINARYOP_H
