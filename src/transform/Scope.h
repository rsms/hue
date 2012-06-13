// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef _HUE_TRANSFORM_SCOPE_INCLUDED
#define _HUE_TRANSFORM_SCOPE_INCLUDED

#include <hue/Text.h>
#include <hue/ast/Type.h>
#include <hue/ast/Expression.h>
#include <hue/ast/Symbol.h>
#include <hue/ast/Structure.h>
#include <string>
#include <map>
#include <deque>
#include <sstream>

namespace hue { namespace transform {

class Scope;
class Scoped;


class Target {
public:
  typedef std::map<const Text, Target> Map;

  static Target Empty;
  
  enum TargetType {
    ScopedValue = 0,
    StructValue,
    StructType,
  } typeID;

  union {
    ast::Node* value; // typeID == ScopedValue
    const ast::Structure::Member* structMember; // typeID == StructValue
    const ast::Type* structMemberType; // typeID == StructType
  };
  union {
    Scope* scope; // typeID == ScopedValue
    const Target* parentTarget; // typeID == StructValue || StructType
  };
  
  Target(TargetType typ = ScopedValue) : typeID(typ), value(0), scope(0) {}

  inline bool isEmpty() const { return value == 0; }

  const ast::Node* astNode() const;
  const ast::Type* resultType() const;
  
  // Find a sub-target
  const Target& lookupSymbol(const Text& name);
  const Target& lookupSymbol(const ast::Type* T, const Text& name);


protected:
  Map targets_;
};


typedef std::deque<Scope*> ScopeStack;

class Scope {
public:
  Scope(Scoped* supervisor);
  virtual ~Scope();

  void defineSymbol(const Text& name, ast::Node *value) {
    Target& target = targets_[name];
    target.value = value;
    target.scope = this;
  }
  // Look up a target only in this scope.
  // Use Visitor::lookupSymbol to lookup stuff in any scope
  const Target& lookupSymbol(const Text& name) {
    Target::Map::const_iterator it = targets_.find(name);
    if (it != targets_.end()) {
      return it->second;
    }
    return Target::Empty;
  }

protected:
  friend class Scoped;
  Scoped* supervisor_;
  Target::Map targets_;
};

class Scoped {
public:
  // Current scope, or 0 if none
  virtual Scope* currentScope() const { return scopeStack_.empty() ? 0 : scopeStack_.back(); }
  virtual Scope* parentScope() const { return scopeStack_.size() > 1 ? 0 : scopeStack_.back() -1; }
  
  // Find a target by name
  const Target& lookupSymbol(const Text& name) {
    // Scan symbol maps starting at top of stack moving down
    ScopeStack::const_reverse_iterator I = scopeStack_.rbegin();
    ScopeStack::const_reverse_iterator E = scopeStack_.rend();
    for (; I != E; ++I) {
      //Scope* scope = *I;
      const Target& target = (*I)->lookupSymbol(name);
      if (!target.isEmpty())
        return target;
    }
    return Target::Empty;
  }

  // Find a target by a potentially nested symbol
  const Target& lookupSymbol(const ast::Symbol& sym);

  inline void defineSymbol(const Text& name, ast::Node *value) {
    currentScope()->defineSymbol(name, value);
  }

private:
  friend class Scope;
  ScopeStack scopeStack_;
};


}} // namespace hue transform
#endif // _HUE_TRANSFORM_SCOPE_INCLUDED
