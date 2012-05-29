// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "Scope.h"

namespace hue { namespace transform {

Target Target::Empty;

Scope::Scope(Scoped* supervisor) : supervisor_(supervisor) {
  supervisor_->scopeStack_.push_back(this);
}
Scope::~Scope() {
  supervisor_->scopeStack_.pop_back();
}

}} // namespace hue transform
