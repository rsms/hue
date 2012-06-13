// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef HUE_AST_SYMBOL_H
#define HUE_AST_SYMBOL_H

#include "Expression.h"

namespace hue { namespace ast {

// Representes a symbolic reference, like "a" or "foo:bar".
class Symbol : public Expression {
public:
  Symbol(const Text &name, bool isPath);

  const Text& name() const { return pathname_.size() == 0 ? Text::Empty : pathname_[0]; }
  const Text::List& pathname() const { return pathname_; }
  bool isPath() const { return pathname_.size() > 1; }
  virtual std::string toString(int level = 0) const;

protected:
  Text::List pathname_;
};


}} // namespace hue::ast
#endif // HUE_AST_SYMBOL_H
