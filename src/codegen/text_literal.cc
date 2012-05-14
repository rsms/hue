// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "_VisitorImplHeader.h"


Value *Visitor::codegenTextLiteral(const ast::TextLiteral *textLit) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  // Array type
  const Text& text = textLit->text();
  ast::Type CoveCharType(ast::Type::Char);
  Type* elementT = IRTypeForASTType(&CoveCharType);
  ArrayType* arrayT = ArrayType::get(elementT, text.size());
  
  // Make Constant values
  std::vector<Constant*> v;
  v.reserve(text.size());
  Text::const_iterator it = text.begin();
  size_t i = 0;
  for (; it != text.end(); ++it, ++i) {
    v.push_back( ConstantInt::get(getGlobalContext(), APInt(32, static_cast<uint64_t>(*it), false)) );
  }
  
  // Package as a constant array
  Constant* arrayV = ConstantArray::get(arrayT, makeArrayRef(v));
  if (arrayV == 0) return error("Failed to create text constant array");
  
  GlobalVariable* arrayGV = createArray(arrayV, "text");
  
  Type* T = PointerType::get(getArrayStructType(elementT), 0);
  return builder_.CreatePointerCast(arrayGV, T);
}

#include "_VisitorImplFooter.h"
