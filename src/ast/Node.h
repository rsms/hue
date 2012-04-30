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
  const enum Type {
    TNode = 0,
    TBlock = 1,
    
    TFunctionInterface = 2,
    TFunction = 3,
    TExternalFunction = 4,
    
    TExpression = 5,
    TIntLiteral = 6,
    TFloatLiteral = 7,
    TSymbol = 8,
    TAssignment = 9,
    TBinaryOp = 10,
    TCall = 11,
    
    _TypeCount
  } type;
  
  Node(Type t = TNode) : type(t) {}
  virtual ~Node() {}
  
  inline bool isFunctionType() const { return type == TFunction || type == TExternalFunction; }
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss; NodeToStringHeader(level, ss); ss << "<Node>";
    return ss.str();
  }
};

}} // namespace rsms.ast
#endif // RSMS_AST_NODE_H
