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



const Target& Scoped::lookupSymbol(const ast::Symbol& sym) {
  // TODO: namespaces
  const Text& name = sym.name();

  if (!sym.isPath()) {
    return lookupSymbol(name);

  } else if (!name.empty()) {
    std::vector<Text> components = name.split(':');
    const Target& target = lookupSymbol(components[0]);

    if (!target.isEmpty()) {
      if (components.size() == 1) {
        return target;
      }

      // Dig into target
      //rlog("components: " << Text("/").join(components));
      std::vector<Text>::const_iterator I = components.begin(), E = components.end();
      ++I;
      Target* target1 = (Target*)&target;
      //rlog("target.typeID: " << target.typeID);

      for (; I != E; ++I) {
        const Target& target2 = target1->lookupSymbol(*I);
        if (target2.isEmpty())
          return target2; // empty
        target1 = (Target*)&target2;
      }

      // OMG we found something
      return const_cast<const Target&>(*target1);
    }
  }

  return Target::Empty;
}


const ast::Node* Target::astNode() const {
  if (typeID == ScopedValue && value) {
    return value;
  } else if (typeID == StructValue && structMember) {
    return structMember->value;
  }
  return 0;
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
        return 0;
      }
    }

    case StructValue: {
      return structMember && structMember->value ? structMember->value->resultType() : 0;
    }

    case StructType: {
      return structMemberType;
    }

    default: return 0;
  }
}


const Target& Target::lookupSymbol(const ast::Type* T, const Text& name) {
  //rlog("Result type: " << T->toString());

  if (T->isStructure()) {
    //rlog("isStructure");
    const ast::StructType* ST = static_cast<const ast::StructType*>(T);
    const ast::Type* memberT = (*ST)[name];

    if (memberT) {
      //rlog("creating Target::StructType");
      Target& target = targets_[name];
      target.typeID = StructType;
      target.parentTarget = this;
      target.structMemberType = memberT;
      return target;
    }
  }

  return Target::Empty;
}


// Find a sub-target by name.
const Target& Target::lookupSymbol(const Text& name) {
  //rlog("Target::lookupSymbol(\"" << name << "\")");

  // First, see if we have an existing target
  Target::Map::const_iterator it = targets_.find(name);
  if (it != targets_.end()) return it->second;

  // Digging in a lazy struct?
  if (typeID == StructType && structMemberType) {
    return lookupSymbol(structMemberType, name);
  }

  // Find node and bail unless its an expression
  const ast::Node* node = astNode();
  if (node == 0 || !node->isExpression()) return Target::Empty;
  const ast::Expression* expr = static_cast<const ast::Expression*>(node);
  
  // Okay, we don't know about name. If we have a value and that value is a
  // struct, dig into it.
  if (expr->isStructure()) {
    const ast::Structure* st = static_cast<const ast::Structure*>(expr);
    const ast::Structure::Member* member = (*st)[name];

    if (member) {
      Target& target = targets_[name];
      target.typeID = StructValue;
      target.parentTarget = this;
      target.structMember = member;
      return target;
    }
    // else {
    //   rlog("Member " << name << " not found in structure " << st->toString());
    // }

  } else {
    const ast::Type* T = expr->resultType();
    return lookupSymbol(T, name);
  }

  return Target::Empty;
}



}} // namespace hue transform
