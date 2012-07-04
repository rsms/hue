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

//
// Logic to deal with symbolic references, potentially nested in scopes.
//

namespace hue { namespace transform {

class Scope;
class Scoped;


class Target {
public:
  typedef std::map<const Text, Target> Map;

  static Target Empty;
  
  enum TargetType {
    ScopedValue = 0,
    StructType,
  } typeID;

  union {
    ast::Node* value; // typeID == ScopedValue
    const ast::Type* structMemberType; // typeID == StructType
  };
  union {
    Scope* scope; // typeID == ScopedValue
    const Target* parentTarget; // typeID == StructType
  };
  
  Target(TargetType typ = ScopedValue) : typeID(typ), value(0), scope(0) {}

  inline bool isEmpty() const { return value == 0; }
  const ast::Type* resultType() const;
  
  // Find a sub-target
  const Target& lookupSymbol(Text::List::const_iterator it, Text::List::const_iterator itend);

protected:
  Map targets_;
};


// This class adds itself to the top of the stack of a supervisor Scoped
// instance when created with a constructor that takes a Scoped instance.
// When a Scope instance is deallocated, is pops itself from any Scoped's
// stack is lives in. This makes it convenient to map stack scopes to Scope
// instances:
//
//   void parseBlock(Block* block) {
//      Scope scope(scopes_); // pushes itself to scopes_ stack
//      for each symboldef:
//         scope.defineSymbol(symboldef.name, symboldef.rhs);
//      // any code inside here can now access these symbols through
//      // scopes_.lookupSymbol(...).
//   }
//   // scope pops itself from scopes_ stack
//
class Scope {
public:
  typedef std::deque<Scope*> Stack;

  Scope(Scoped* supervisor);
  virtual ~Scope();

  // Define a symbol as being rooted in this scope.
  // *name* must not be a pathname.
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
  virtual Scope* rootScope() const { return scopeStack_.size() == 0 ? 0 : scopeStack_.front(); }
  
  // Find a target by name
  const Target& lookupSymbol(const Text& name);

  // Find a target by a potentially nested symbol
  const Target& lookupSymbol(const ast::Symbol& sym);

  inline void defineSymbol(const Text& name, ast::Node *value) {
    currentScope()->defineSymbol(name, value);
  }

private:
  friend class Scope;
  Scope::Stack scopeStack_;
};


}} // namespace hue transform
#endif // _HUE_TRANSFORM_SCOPE_INCLUDED
