#ifndef RSMS_AST_CONDITIONALS_H
#define RSMS_AST_CONDITIONALS_H
#include "Node.h"
#include "Block.h"
#include <vector>

namespace rsms { namespace ast {

class Conditional : public Expression {
public:
  Conditional() : Expression(TConditional), testExpression_(0), trueBlock_(0), falseBlock_(0) {}
  
  Expression* setTestExpression(Expression* testExpression) { return testExpression_ = testExpression; }
  inline Expression* testExpression() const { return testExpression_; }

  Block* setTrueBlock(Block* trueBlock) { return trueBlock_ = trueBlock; }
  inline Block* trueBlock() const { return trueBlock_; }
  
  Block* setFalseBlock(Block* falseBlock) { return falseBlock_ = falseBlock; }
  inline Block* falseBlock() const { return falseBlock_; }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Conditional " << (testExpression_ ? testExpression_->toString(level+1) : "<nil>");
    NodeToStringHeader(level+1, ss);
    ss << "then: " << (trueBlock_ ? trueBlock_->toString(level+1) : "<nil>");
    NodeToStringHeader(level+1, ss);
    ss << "else: " << (falseBlock_ ? falseBlock_->toString(level+1) : "<nil>");
    ss << ">";
    return ss.str();
  }
  
private:
  Expression* testExpression_;
  Block* trueBlock_;
  Block* falseBlock_;
};

}} // namespace rsms.ast
#endif // RSMS_AST_CONDITIONALS_H
