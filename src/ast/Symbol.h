// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

// Expressions
#ifndef HUE_AST_SYMBOL_H
#define HUE_AST_SYMBOL_H

#include "Expression.h"
#include <assert.h>

namespace hue { namespace ast {

class FunctionType;

// Referencing a symbol, like "a".
class Symbol : public Expression {
public:
  Symbol(const Text &name) : Expression(TSymbol), name_(name), value_(0) {}
  const Text& name() const { return name_; }

  Node* value() const { return value_; }
  void setValue(Node* value) { value_ = value; }

  virtual const Type *resultType() const;

  virtual void setResultType(const Type* T);

  virtual std::string toString(int level = 0) const {
    return name_.UTF8String();
    // std::ostringstream ss;
    // ss << "<Symbol name=" << name_;
    // if (value_) {
    //   ss << " -> " << value_->toString(level);
    // }
    // ss << '>';
    // return ss.str();
  }
protected:
  Text name_;
  Node* value_;
};


}} // namespace hue::ast
#endif // HUE_AST_SYMBOL_H
