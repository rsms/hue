// Base class for all expression nodes.
#ifndef RSMS_AST_NODE_H
#define RSMS_AST_NODE_H
#include <sstream>

namespace rsms { namespace ast {

inline static void NodeToStringHeader(int level, std::ostream& ss) {
  if (level > 0) {
    ss << std::endl;
    for (int i=level; --i; ) ss << "  ";
  }
}

class Node {
public:
  enum NodeTypeID {
    TNode,
    TBlock,
    
    TFunctionType,
    TFunction,
    TExternalFunction,
    
    TExpression,
    TIntLiteral,
    TFloatLiteral,
    TBoolLiteral,
    TDataLiteral,
    TTextLiteral,
    
    TSymbol,
    TAssignment,
    TBinaryOp,
    TCall,
    TConditional,
    
    _TypeCount
  };
  
  Node(NodeTypeID t = TNode) : type_(t) {}
  virtual ~Node() {}
  
  inline const NodeTypeID& nodeTypeID() const { return type_; }
  inline bool isFunctionType() const { return type_ == TFunction || type_ == TExternalFunction; }
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss; NodeToStringHeader(level, ss); ss << "<Node>";
    return ss.str();
  }
  
private:
  const NodeTypeID type_;
};

}} // namespace rsms.ast
#endif // RSMS_AST_NODE_H
