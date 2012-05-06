#ifndef RSMS_AST_BLOCK_H
#define RSMS_AST_BLOCK_H
#include "Expression.h"
#include <vector>

namespace rsms { namespace ast {

class Block : public Expression {
public:
  typedef std::vector<Node*> NodeList;
  Block() : Expression(TBlock) {}
  Block(Node *node) : Expression(TBlock), nodes_(1, node) {}
  Block(const NodeList &nodes) : Expression(TBlock), nodes_(nodes) {}
  
  virtual ~Block() {
    // TODO: delete all nodes
  }
  
  const NodeList& nodes() const { return nodes_; };
  
  void addNode(Node *node) { nodes_.push_back(node); };

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<Block [" << nodes_.size() << "](";
    NodeList::const_iterator it1;
    it1 = nodes_.begin();
    if (it1 < nodes_.end()) {
      //NodeToStringHeader(level, ss);
      ss << (*it1)->toString(level+1);
      it1++;
    }
    for (; it1 < nodes_.end(); it1++) {
      //NodeToStringHeader(level, ss);
      ss << ", " << (*it1)->toString(level+1);
    }
    ss << ")>";
    return ss.str();
  }
  
private:
  NodeList nodes_;
};

}} // namespace rsms.ast
#endif // RSMS_AST_BLOCK_H
