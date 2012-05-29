// Copyright (c) 2012, Rasmus Andersson. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.
#ifndef HUE__AST_BLOCK_H
#define HUE__AST_BLOCK_H
#include "Expression.h"
#include "../Logger.h"
#include <assert.h>

namespace hue { namespace ast {

class Block : public Expression {
public:
  Block() : Expression(TBlock) {}
  Block(Expression *expression) : Expression(TBlock), expressions_(1, expression) {}
  Block(const ExpressionList &expressions) : Expression(TBlock), expressions_(expressions) {}
  
  const ExpressionList& expressions() const { return expressions_; };
  void addExpression(Expression *expression) { expressions_.push_back(expression); };

  virtual const Type *resultType() const {
    if (expressions_.size() != 0) {
      return expressions_.back()->resultType();
    } else {
      return resultType_; // Type::Unknown
    }
  }

  virtual void setResultType(const Type* T) {
    assert(T->isUnknown() == false); // makes no sense to set T=unknown
    // Call setResultType with T for last expression if the last expression is unknown
    if (expressions_.size() != 0) {
      expressions_.back()->setResultType(T);
    }
  }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "<Block (";
    ExpressionList::const_iterator it1;
    it1 = expressions_.begin();
    if (it1 < expressions_.end()) {
      //NodeToStringHeader(level, ss);
      ss << (*it1)->toString(level+1);
      it1++;
    }
    for (; it1 < expressions_.end(); it1++) {
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
  ExpressionList expressions_;
};

}} // namespace hue.ast
#endif // HUE__AST_BLOCK_H
