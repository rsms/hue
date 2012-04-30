#ifndef RSMS_AST_CONDITIONALS_H
#define RSMS_AST_CONDITIONALS_H
#include "Node.h"
#include "Block.h"
#include <vector>

namespace rsms { namespace ast {

class Conditional : public Expression {
public:
  class ConditionalBranch {
  public:
    explicit ConditionalBranch(Expression* testExpr, Block* block) : testExpr_(testExpr), block_(block) {}
    std::string toString(int level = 0) const {
      std::ostringstream ss;
      NodeToStringHeader(level, ss);
      ss << "<IF " << (testExpr_ ? testExpr_->toString(level+1) : "<nil>")
         << " THEN " << (block_ ? block_->toString(level+1) : "<nil>")
         << '>';
      return ss.str();
    }
  private:
    Expression* testExpr_;
    Block* block_;
  };
  
  typedef std::vector<ConditionalBranch> ConditionalBranchList;
  
  Conditional() : Expression(TConditional), defaultBlock_(0) {}
  
  void addConditionalBranch(const ConditionalBranch& cb) { conditionalBranches_.push_back(cb); }
  void addConditionalBranch(Expression* testExpr, Block* block) {
    addConditionalBranch(ConditionalBranch(testExpr, block));
  }
  inline const ConditionalBranchList& conditionalBranches() const { return conditionalBranches_; }
  
  void setDefaultBlock(Block* defaultBlock) { defaultBlock_ = defaultBlock; }
  inline Block* defaultBlock() const { return defaultBlock_; }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Conditional ";
    ConditionalBranchList::const_iterator it1;
    it1 = conditionalBranches_.begin();
    if (it1 < conditionalBranches_.end()) {
      ss << (*it1).toString(level+1);
      it1++;
    }
    for (; it1 < conditionalBranches_.end(); it1++) {
      ss << ", " << (*it1).toString(level+1);
    }
    if (defaultBlock_) {
      NodeToStringHeader(level+1, ss);
      ss << "default: " << defaultBlock_->toString(level+2);
    }
    ss << ">";
    return ss.str();
  }
  
private:
  ConditionalBranchList conditionalBranches_;
  Block* defaultBlock_;
};

}} // namespace rsms.ast
#endif // RSMS_AST_CONDITIONALS_H
