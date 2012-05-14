// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

// Base class for all expression nodes.
#ifndef HUE__AST_NODE_H
#define HUE__AST_NODE_H

#include <sstream>
#include <vector>

namespace hue { namespace ast {

inline static void NodeToStringHeader(int level, std::ostream& ss) {
  if (level > 0) {
    ss << std::endl;
    for (int i=level; --i; ) ss << "  ";
  }
}

class Node;
typedef std::vector<Node*> NodeList;

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
    TListLiteral,
    
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

}} // namespace hue.ast
#endif // HUE__AST_NODE_H
