// Copyright (c) 2012, Rasmus Andersson. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.
#ifndef HUE__AST_BLOCK_H
#define HUE__AST_BLOCK_H
#include "Expression.h"

namespace hue { namespace ast {

class Block : public Expression {
public:
  Block() : Expression(TBlock) {}
  Block(Node *node) : Expression(TBlock), nodes_(1, node) {}
  Block(const NodeList &nodes) : Expression(TBlock), nodes_(nodes) {}
  
  const NodeList& nodes() const { return nodes_; };
  void addNode(Node *node) { nodes_.push_back(node); };

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<Block #" << nodes_.size() << " (";
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

  // TODO:
  // virtual std::string toHueSource() const {
  //   
  // }
  
private:
  NodeList nodes_;
};

}} // namespace hue.ast
#endif // HUE__AST_BLOCK_H
