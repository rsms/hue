// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "StructType.h"
#include "Block.h"
#include "../Mangle.h"

namespace hue { namespace ast {

//static
// StructType* StructType::get(const TypeList& types) {
//   // TODO reuse
//   StructType* ST = new StructType;
//   ST->types_.assign(types.begin(), types.end());
//   return ST;
// }


//static
StructType* StructType::get(const Member::List& members) {
  // TODO reuse
  StructType* ST = new StructType;
  ST->types_.reserve(members.size());

  for (Member::List::const_iterator I = members.begin(), E = members.end(); I != E; ++I) {
    ST->nameMap_[(*I).name] = ST->types_.size();
    ST->types_.push_back((*I).type);
  }

  return ST;
}


std::string StructType::toString() const {
  std::string s("<{");
  for (size_t i = 0, L = size(); i < L; ++i ) {
    s += types_[i]->toString();
    if (i != L-1) s += ", ";
  }
  return s + "}>";
}


std::string StructType::canonicalName() const {
  return std::string("type.") + mangle(*this);
}


const Type* StructType::operator[](const Text& name) const {
  NameMap::const_iterator it = nameMap_.find(name);
  if (it != nameMap_.end()) {
    return types_[it->second];
  } else {
    return 0;
  }
}

}} // namespace hue::ast
