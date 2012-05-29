// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef _HUE_TRANSFORM_SCOPE_INCLUDED
#define _HUE_TRANSFORM_SCOPE_INCLUDED

#include <hue/Text.h>
#include <hue/ast/Expression.h>
#include <string>
#include <map>
#include <deque>
#include <sstream>

namespace hue { namespace transform {

class Scope;
class Scoped;

class Target {
public:
  static Target Empty;
  ast::Node *value;
  Scope *scope;

  inline bool isEmpty() const { return value == 0 || scope == 0; }
  
  Target() : value(0), scope(0) {}
  Target(ast::Node *V, Scope* S = 0)
    : value(V), scope(S) {}
};

typedef std::map<Text, Target> TargetMap;
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
  const Target& lookupSymbol(const Text& name) const {
    TargetMap::const_iterator it = targets_.find(name);
    if (it != targets_.end()) return it->second;
    return Target::Empty;
  }

protected:
  friend class Scoped;
  Scoped* supervisor_;
  TargetMap targets_;
};

class Scoped {
public:
  // Current scope, or 0 if none
  virtual Scope* currentScope() const { return scopeStack_.empty() ? 0 : scopeStack_.back(); }
  virtual Scope* parentScope() const { return scopeStack_.size() > 1 ? 0 : scopeStack_.back() -1; }
  
  // Find a symbol
  const Target& lookupSymbol(const Text& name) const {
    // Scan symbol maps starting at top of stack moving down
    ScopeStack::const_reverse_iterator I = scopeStack_.rbegin();
    ScopeStack::const_reverse_iterator E = scopeStack_.rend();
    for (; I != E; ++I) {
      Scope* scope = *I;
      TargetMap::const_iterator it = scope->targets_.find(name);
      if (it != scope->targets_.end()) {
        return it->second;
      }
    }
    return Target::Empty;
  }

  inline void defineSymbol(const Text& name, ast::Node *value) {
    currentScope()->defineSymbol(name, value);
  }

private:
  friend class Scope;
  ScopeStack scopeStack_;
};


}} // namespace hue transform
#endif // _HUE_TRANSFORM_SCOPE_INCLUDED
