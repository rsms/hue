#ifndef HUE__AST_LIST_LITERAL_H
#define HUE__AST_LIST_LITERAL_H
#include "Block.h"

namespace hue { namespace ast {

class ListLiteral : public Expression {
public:
  ListLiteral() : Expression(TListLiteral) {}
  const NodeList& nodes() const { return nodes_; };
  void addNode(Node *node) { nodes_.push_back(node); };
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<ListLiteral #" << nodes_.size() << " [";
    NodeList::const_iterator it1;
    it1 = nodes_.begin();
    if (it1 < nodes_.end()) {
      ss << (*it1)->toString(level+1);
      it1++;
    }
    for (; it1 < nodes_.end(); it1++) {
      ss << ", " << (*it1)->toString(level+1);
    }
    ss << "]>";
    return ss.str();
  }
  
private:
  NodeList nodes_;
};

}} // namespace hue.ast
#endif // HUE__AST_LIST_LITERAL_H
