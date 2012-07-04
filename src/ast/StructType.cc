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
    ST->nameToIndexMap_[(*I).name] = ST->types_.size();
    ST->types_.push_back((*I).type);
  }

  return ST;
}


std::string StructType::toString() const {
  // This is kind of inefficient...
  // Create index-to-name map
  size_t i = 0;
  std::vector<const Text*> orderedNames;
  orderedNames.reserve(types_.size());

  for (NameMap::const_iterator I = nameToIndexMap_.begin(), E = nameToIndexMap_.end();
       I != E; ++I)
  {
    orderedNames[i++] = &(I->first);
  }

  std::string s("{");
  for (size_t i = 0, L = size(); i < L; ++i ) {
    s += orderedNames[i]->UTF8String() + ":" + types_[i]->toString();
    if (i != L-1) s += " ";
  }
  return s + "}";
}


std::string StructType::canonicalName() const {
  return std::string("type.") + mangle(*this);
}


size_t StructType::indexOf(const Text& name) const {
  NameMap::const_iterator it = nameToIndexMap_.find(name);
  if (it != nameToIndexMap_.end()) {
    return it->second;
  } else {
    return SIZE_MAX;
  }
}


const Type* StructType::operator[](const Text& name) const {
  NameMap::const_iterator it = nameToIndexMap_.find(name);
  if (it != nameToIndexMap_.end()) {
    return types_[it->second];
  } else {
    return 0;
  }
}

}} // namespace hue::ast
