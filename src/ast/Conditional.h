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

  virtual const Type *resultType() const {
    assert(trueBlock_);
    const Type* trueT = trueBlock_->resultType();
    const Type* falseT = falseBlock_->resultType();
    return Type::highestFidelity(trueT, falseT);
  }

  virtual void setResultType(const Type* T) {
    assert(trueBlock_);
    trueBlock_->setResultType(T);
    assert(falseBlock_);
    falseBlock_->setResultType(T);
  }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    //NodeToStringHeader(level, ss);
    ss << "(if " << (testExpression_ ? testExpression_->toString(level) : "nil");
    NodeToStringHeader(level+1, ss);
    ss << (trueBlock_ ? trueBlock_->toString(level+1) : "nil");
    if (level == 0) ss << "\n"; else NodeToStringHeader(level, ss);
    ss << "else ";
    NodeToStringHeader(level+1, ss);
    ss << (falseBlock_ ? falseBlock_->toString(level+1) : "nil")
       << ")";
    return ss.str();
  }
  
private:
  Expression* testExpression_;
  Block* trueBlock_;
  Block* falseBlock_;
};

}} // namespace hue.ast
#endif // HUE__AST_CONDITIONALS_H
