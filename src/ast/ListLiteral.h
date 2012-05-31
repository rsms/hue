// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#ifndef HUE__AST_LIST_LITERAL_H
#define HUE__AST_LIST_LITERAL_H
#include "Block.h"

namespace hue { namespace ast {

class ListLiteral : public Expression {
public:
  ListLiteral() : Expression(TListLiteral, ArrayType::get(UnknownType)) {}
  const ExpressionList& expressions() const { return expressions_; };
  void addExpression(Expression *expression) { expressions_.push_back(expression); };
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<ListLiteral " << expressions_.size() << "[";
    ExpressionList::const_iterator it1;
    it1 = expressions_.begin();
    if (it1 < expressions_.end()) {
      ss << (*it1)->toString(level+1);
      it1++;
    }
    for (; it1 < expressions_.end(); it1++) {
      ss << ", " << (*it1)->toString(level+1);
    }
    ss << "]>";
    return ss.str();
  }
  
private:
  ExpressionList expressions_;
};

}} // namespace hue.ast
#endif // HUE__AST_LIST_LITERAL_H
