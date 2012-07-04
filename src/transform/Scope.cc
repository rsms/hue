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
  if (target.isEmpty() || pathname.size() == 1) {
    return target;
  }

  // Dig into target
  //rlog("digging into target 0: " << target.toString() << ", pathname: " << Text(", ").join(pathname));
  return const_cast<Target&>(target).lookupSymbol(pathname.begin()+1, pathname.end());
}


const Target& Target::lookupSymbol(Text::List::const_iterator it, Text::List::const_iterator itend) {
  const Text& name = *it;
  ++it;
  //rlog("<Target " << toString() << ">::lookupSymbol(\"" << name << "\", itend)");

  Target* target = 0;

  // Already resolved?
  Target::Map::iterator MI = targets_.find(name);
  if (MI != targets_.end()) {
    target = &MI->second;

  } else {
    // Dig further based on our type
    const ast::Type* T = resultType();
    if (T == 0) {
      return Target::Empty;
    }

    if (T->isFunction()) {
      Target& t = targets_[name];
      t.typeID = FunctionType;
      if (it == itend) {
        t.type = T;
      } //else rlog("Target::lookupSymbol: Unexpected leaf when expecting a branch");
      return t;

    } else if (!T->isStructure()) {
      Target& t = targets_[name];
      t.typeID = LeafType;
      if (it == itend) {
        t.type = T;
      } //else rlog("Target::lookupSymbol: Unexpected leaf when expecting a branch");
      return t;
    }

    // Else: T is a StructType
    const ast::StructType* ST = static_cast<const ast::StructType*>(T);

    // Lookup struct member by name
    const ast::Type* memberT = (*ST)[name];
    if (memberT == 0) {
      rlogw("Unknown symbol \"" << name << "\" in struct " << ST->toString());
      return Target::Empty;
    }
    //rloge("ST MEMBER for " << name << " => " << memberT->toString());

    // Register target for name
    Target& t = targets_[name];
    target = &t;
    if (memberT->isStructure()) {
      t.typeID = StructType;
      t.type = memberT;

    } else if (memberT->isFunction()) {
      t.typeID = FunctionType;
      if (it == itend) {
        t.type = memberT;
      } //else rlog("Target::lookupSymbol: Unexpected function leaf when expecting a branch");

    } else {
      t.typeID = LeafType;
      if (it == itend) {
        t.type = memberT;
      } //else rlog("Target::lookupSymbol: Unexpected value leaf when expecting a branch");
    }
  }

  // Are we the leaf?
  if (it == itend) {
    //rlog("Leaf found! -> " << (target ? target->toString() : std::string("<nil>")) );
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

      } else if (value->isValue()) {
        const ast::Value* var = static_cast<const ast::Value*>(value);
        return var->resultType();

      } else {
        return &ast::UnknownType;
      }
    }

    case LeafType:
    case StructType:
    case FunctionType:
      return type;

    default: return 0;
  }
}


std::string Target::toString() const {
  if (hasValue()) {
    return "value:" + (value ?
      (std::string(value->typeName()) + ":" + value->toString()) : std::string("<null>"));
  } else if (hasType()) {
    return "type:" + (type ? type->toString() : std::string("<null>"));
  } else {
    return "<null>";
  }
}


}} // namespace hue transform
