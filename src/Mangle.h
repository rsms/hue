// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef _HUE_CODEGEN_MANGLE_INCLUDED
#define _HUE_CODEGEN_MANGLE_INCLUDED

#include "ast/Type.h"
#include "ast/Function.h"

#include <string>
#include <sstream>
#include <assert.h>

namespace hue {

// MangledType = <ASCII string>
inline std::string mangle(const ast::Type& type) {
  return type.toMangleID();
}
inline ast::Type demangle(const std::string& mangleID) {
  return ast::Type::createFromMangleID(mangleID);
}

// MangledTypeList = MangledType*
std::string mangle(const ast::TypeList& types) {
  std::ostringstream ss;
  ast::TypeList::const_iterator it = types.begin();
  for (; it != types.end(); ++it) {
    ss << mangle(*(*it));
  }
  return ss.str();
}

// MangledVarList = MangledTypeList
std::string mangle(const ast::VariableList& vars) {
  std::ostringstream ss;
  ast::VariableList::const_iterator it = vars.begin();
  for (; it != vars.end(); ++it) {
    assert((*it)->type() != 0);
    ss << mangle(*(*it)->type());
  }
  return ss.str();
}

// MangledFuncType = '$' MangledVarList '$' MangledTypeList
//
// ^(a Int) Int -> $I$I
//
std::string mangle(const ast::FunctionType& funcType) {
  std::string mname = "$";
  ast::VariableList* args = funcType.args();
  if (args) mname += mangle(*args);
  mname += '$';
  ast::Type* returnType = funcType.returnType();
  if (returnType) mname += mangle(*returnType);
  return mname;
}

} // namespace hue
#endif // _HUE_CODEGEN_MANGLE_INCLUDED
