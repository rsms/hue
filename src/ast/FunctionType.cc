// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "FunctionType.h"
#include <assert.h>

namespace hue { namespace ast {

std::string FunctionType::toString(int level) const {
  std::ostringstream ss;
  //NodeToStringHeader(level, ss);
  ss << "func (";
  if (args_) {
    VariableList::const_iterator it1;
    it1 = args_->begin();
    if (it1 < args_->end()) { ss << (*it1)->toString(); it1++; }
    for (; it1 < args_->end(); it1++) { ss << ", " << (*it1)->toString(); }
  }
  ss << ")";

  if (resultType_) ss << ' ' << resultType_->toString();
  return ss.str();
}


std::string FunctionType::toHueSource() const {
  std::ostringstream ss;
  ss << "func ";
  
  if (args_ && !args_->empty()) {
    ss << '(';

    for (VariableList::iterator I = args_->begin(), E = args_->end(); I != E; ++I) {
      assert((*I) != 0);
      if ((*I)->hasUnknownType()) {
        ss << "?";
      } else {
        ss << (*I)->type()->toHueSource();
      }
      if (I != E) ss << ", ";
    }
    ss << ')';
  }
  
  if (resultType_)
    ss << ' ' << resultType_->toHueSource();
  
  return ss.str();
}

}} // namespace hue::ast
