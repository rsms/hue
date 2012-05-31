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
  Block(const Type* resultType) : Expression(TBlock, resultType) {}
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
    //ss << "(";
    ExpressionList::const_iterator I = expressions_.begin(), E = expressions_.end();
    if (I != E) {
      ss << (*I)->toString(level);
      ++I;
    }
    for (;I != E; ++I) {
      ss << " " << (*I)->toString(level);
    }
    //ss << ")";
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
