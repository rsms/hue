// Base class for all expression nodes.
#ifndef RSMS_AST_NODE_H
#define RSMS_AST_NODE_H
#include <sstream>

// #define RETURN_FORMAT_STRING(fmt, ...) do { \
//   const size_t bufsize = 512; \
//   char buf[bufsize+1]; \
//   snprintf(buf, bufsize, fmt, __VA_ARGS__); \
//   return std::string(buf); \
// } while (0)

inline static void NodeToStringHeader(int level, std::ostream& ss) {
  if (level > 0) {
    ss << std::endl;
    for (int i=level; --i; ) ss << "  ";
  }
}

namespace rsms { namespace ast {

class Node {
public:
  const enum Type {
    TNode = 0,
    TBlock,
    
    TFunctionInterface,
    TFunction,
    TExternalFunction,
    
    TExpression,
    TIntLiteralExpression,
    TFloatLiteralExpression,
    TSymbolExpression,
    TAssignmentExpression,
    TBinaryExpression,
    TCallExpression,
    
    _TypeCount
  } type;
  
  Node(Type t = TNode) : type(t) {}
  virtual ~Node() {}
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<Node>";
    return ss.str();
  }
};

}} // namespace rsms.ast
#endif // RSMS_AST_NODE_H
