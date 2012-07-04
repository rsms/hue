// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "Scope.h"
#include "../Logger.h"

namespace hue { namespace transform {

Target Target::Empty;

Scope::Scope(Scoped* supervisor) : supervisor_(supervisor) {
  supervisor_->scopeStack_.push_back(this);
}
Scope::~Scope() {
  supervisor_->scopeStack_.pop_back();
}


const Target& Scoped::lookupSymbol(const Text& name) {
  // Scan symbol maps starting at top of stack moving down
  Scope::Stack::const_reverse_iterator I = scopeStack_.rbegin();
  Scope::Stack::const_reverse_iterator E = scopeStack_.rend();
  for (; I != E; ++I) {
    //Scope* scope = *I;
    const Target& target = (*I)->lookupSymbol(name);
    if (!target.isEmpty())
      return target;
  }
  return Target::Empty;
}


const Target& Scoped::lookupSymbol(const ast::Symbol& sym) {
  const Text::List& pathname = sym.pathname();
  assert(pathname.size() != 0);
  //rlog("pathname: " << Text("/").join(pathname));

  // Lookup root
  const Target& target = lookupSymbol(pathname[0]);
  if (target.isEmpty() || pathname.size() == 1) return target;

  // Dig into target
  return const_cast<Target&>(target).lookupSymbol(pathname.begin()+1, pathname.end());
}


const Target& Target::lookupSymbol(Text::List::const_iterator it, Text::List::const_iterator itend) {
  const Text& name = *it;
  ++it;
  //rlog("Target::lookupSymbol(\"" << name << "\", itend)");

  Target* target = 0;

  // Already resolved?
  Target::Map::iterator MI = targets_.find(name);
  if (MI != targets_.end()) {
    target = &MI->second;

  } else {
    // Dig further based on our type
    const ast::Type* T = resultType();
    if (T == 0 || !T->isStructure())
      return Target::Empty;

    // Else: T is a StructType
    const ast::StructType* ST = static_cast<const ast::StructType*>(T);

    // Lookup struct member by name
    const ast::Type* memberT = (*ST)[name];
    if (memberT == 0) {
      //rlogw("Unknown symbol \"" << name << "\" in struct " << ST->toString());
      return Target::Empty;
    }

    // Register target for name
    Target& t = targets_[name];
    t.typeID = StructType;
    t.parentTarget = this; // currently unused
    t.structMemberType = memberT;
    target = &t;
  }

  // Are we the leaf?
  if (it == itend) {
    //rlog("Leaf found! -> " << (target->resultType() ? target->resultType()->toString() : std::string("<nil>")) );
    return *target;
  } else {
    return target->lookupSymbol(it, itend);
  }
}


const ast::Type* Target::resultType() const {
  switch (typeID) {

    case ScopedValue: {
      if (value == 0) return 0;
      if (value->isExpression()) {
        const ast::Expression* expr = static_cast<const ast::Expression*>(value);
        return expr->resultType();
      } else if (value->isVariable()) {
        const ast::Variable* var = static_cast<const ast::Variable*>(value);
        return var->type();
      } else {
        return &ast::UnknownType;
      }
    }

    case StructType: {
      return structMemberType;
    }

    default: return 0;
  }
}



}} // namespace hue transform
