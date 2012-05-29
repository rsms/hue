// Copyright (c) 2012, Rasmus Andersson. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.
#ifndef HUE__AST_CONDITIONALS_H
#define HUE__AST_CONDITIONALS_H
#include "Expression.h"
#include "Block.h"
#include <assert.h>
#include <vector>

namespace hue { namespace ast {

class Conditional : public Expression {
public:
  Conditional() : Expression(TConditional), testExpression_(0), trueBlock_(0), falseBlock_(0) {}
  
  Expression* testExpression() const { return testExpression_; }
  void setTestExpression(Expression* expr) { testExpression_ = expr; }

  Block* trueBlock() const { return trueBlock_; }
  void setTrueBlock(Block* B) { trueBlock_ = B; }

  Block* falseBlock() const { return falseBlock_; }
  void setFalseBlock(Block* B) { falseBlock_ = B; }

  virtual Type *resultType() const {
    assert(trueBlock_);
    Type* BT = trueBlock_->resultType();
    if (BT && !BT->isUnknown())
      return BT;

    assert(falseBlock_);
    BT = falseBlock_->resultType();
    if (BT && !BT->isUnknown())
      return BT;

    return resultType_; // Type::Unknown
  }

  virtual void setResultType(Type* T) {
    assert(trueBlock_);
    Type* BT = trueBlock_->resultType();
    if (!BT || BT->isUnknown()) {
      trueBlock_->setResultType(T);
    }

    assert(falseBlock_);
    BT = falseBlock_->resultType();
    if (!BT || BT->isUnknown()) {
      falseBlock_->setResultType(T);
    }
  }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Conditional "
       << "if " << (testExpression_ ? testExpression_->toString(level+1) : "<nil>");
    NodeToStringHeader(level+1, ss);
    ss << "then " << (trueBlock_ ? trueBlock_->toString(level+1) : "<nil>");
    NodeToStringHeader(level+1, ss);
    ss << "else " << (falseBlock_ ? falseBlock_->toString(level+1) : "<nil>")
       << ">";
    return ss.str();
  }
  
private:
  Expression* testExpression_;
  Block* trueBlock_;
  Block* falseBlock_;
};

}} // namespace hue.ast
#endif // HUE__AST_CONDITIONALS_H
