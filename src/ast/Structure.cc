// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "Structure.h"

namespace hue { namespace ast {

// size_t Structure::indexOf(const Text& name) const {
//   NameMap::const_iterator it = nameMap_.find(name);
//   if (it != nameMap_.end()) {
//     return it->second;
//   } else {
//     return SIZE_MAX;
//   }
// }

void Structure::update() {
  const ExpressionList& expressions = block_->expressions();
  
  StructType::Member::List stmembers;
  stmembers.reserve(expressions.size());

  members_.clear();

  ExpressionList::const_iterator I = expressions.begin(), E = expressions.end();
  for (;I != E; ++I) {
    const Expression* expr = *I;
    assert(expr->isAssignment());
    const Assignment* ass = static_cast<const Assignment*>(expr);
    const Text& name = ass->variable()->name();

    size_t index = stmembers.size();
    stmembers.push_back(StructType::Member(ass->resultType(), name));

    Member& member = members_[name];
    member.index = index;
    member.value = ass->rhs();
  }

  structType_ = StructType::get(stmembers);
}


const Structure::Member* Structure::operator[](const Text& name) const {
  MemberMap::const_iterator it = members_.find(name);
  if (it != members_.end()) {
    return &(it->second);
  } else {
    return 0;
  }
}



}} // namespace hue::ast
