// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef _HUE_CODEGEN_MANGLE_INCLUDED
#define _HUE_CODEGEN_MANGLE_INCLUDED

#include "ast/Type.h"
#include "ast/Function.h"

#include <llvm/Type.h>
#include <llvm/DerivedTypes.h>

namespace hue {

std::string mangle(const ast::TypeList& types);
std::string mangle(const ast::VariableList& vars);
std::string mangle(const ast::FunctionType& funcType);
std::string mangle(const hue::ast::Type& T);
std::string mangle(llvm::Type* T);
std::string mangle(llvm::FunctionType* FT);

ast::Type demangle(const std::string& mangleID);

} // namespace hue
#endif // _HUE_CODEGEN_MANGLE_INCLUDED
