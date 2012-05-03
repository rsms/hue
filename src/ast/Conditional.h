#ifndef RSMS_AST_CONDITIONALS_H
#define RSMS_AST_CONDITIONALS_H
#include "Node.h"
#include "Block.h"
#include <vector>

namespace rsms { namespace ast {

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
  inline const BranchList& branches() const { return branches_; }
  
  Block* defaultBlock() const { return defaultBlock_; }
  void setDefaultBlock(Block* B) { defaultBlock_ = B; }

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

}} // namespace rsms.ast
#endif // RSMS_AST_CONDITIONALS_H
