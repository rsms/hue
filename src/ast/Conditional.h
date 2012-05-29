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
  class Branch {
  public:
    Branch(Expression* a, Block* b) : testExpression(a), block(b) {}
    Expression* testExpression;
    Block* block;
    std::string toString(int level = 0) const {
      std::ostringstream ss;
      ss << (testExpression ? testExpression->toString(level) : "<nil>")
         << " -> " << (block ? block->toString(level) : "<nil>");
      return ss.str();
    }
  };
  typedef std::vector<Branch> BranchList;
  Conditional() : Expression(TConditional), defaultBlock_(0) {}
  
  void addBranch(Expression* test1, Block* block1) {
    branches_.push_back(Branch(test1, block1));
  }
  const BranchList& branches() const { return branches_; }
  BranchList& branches() { return branches_; }
  
  Block* defaultBlock() const { return defaultBlock_; }
  void setDefaultBlock(Block* B) { defaultBlock_ = B; }

  virtual Type *resultType() const {
    // Return first branch that has known type
    for (BranchList::const_iterator I = branches_.begin(), E = branches_.end(); I != E; ++I) {
      const Branch& branch = *I;
      Type* BT = branch.block->resultType();
      if (BT && !BT->isUnknown())
        return BT;
    }

    assert(defaultBlock_);
    Type* BT = defaultBlock_->resultType();
    if (BT && !BT->isUnknown())
      return BT;

    return resultType_; // Type::Unknown
  }

  virtual void setResultType(Type* T) {
    // call setResultType on each branch block that has an unknown type
    for (BranchList::iterator I = branches_.begin(), E = branches_.end(); I != E; ++I) {
      Branch& branch = *I;
      Type* BT = branch.block->resultType();
      if (!BT || BT->isUnknown()) {
        branch.block->setResultType(T);
      }
    }

    assert(defaultBlock_);
    Type* BT = defaultBlock_->resultType();
    if (!BT || BT->isUnknown()) {
      defaultBlock_->setResultType(T);
    }
  }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Conditional ";
    BranchList::const_iterator it = branches_.begin();
    for (; it != branches_.end(); ++ it) {
      ss << (*it).toString(level+1);
    }
    NodeToStringHeader(level+1, ss);
    ss << (defaultBlock_ ? defaultBlock_->toString(level+1) : "<nil>") << ">";
    return ss.str();
  }
  
private:
  BranchList branches_;
  Block* defaultBlock_;
};

}} // namespace hue.ast
#endif // HUE__AST_CONDITIONALS_H
