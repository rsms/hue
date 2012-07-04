// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "_VisitorImplHeader.h"

Value *Visitor::codegenSymbol(const ast::Symbol *symbol) {
  DEBUG_TRACE_LLVM_VISITOR;
  assert(symbol != 0);

  // Get and check pathname
  const Text::List& pathname = symbol->pathname();
  if (pathname.size() == 0)
    return error((std::string("Unknown symbol \"") + symbol->toString() + "\""));

  // Lookup symbol
  const SymbolTarget& target = lookupSymbol(pathname[0]);
  if (target.empty())
    return error((std::string("Unknown symbol \"") + symbol->toString() + "\""));
  
  // Extract the targets value
  Value* V = target.value;

  // Deep?
  if (pathname.size() > 1) {
    const ast::Type* hueType = target.hueType;
    assert(hueType != 0);
    //ArrayRef<Value*> gepPath
    //size_t gepIndex = 0;
    //size_t gepIndexCount = pathname.size();
    //Value* GEPPath[gepIndexCount * 2];
    //Value* i32_0V = builder_.getInt32(0);

    for (Text::List::const_iterator I = pathname.begin()+1, E = pathname.end(); I != E; ++I) {
      //rlog("Digging into " << hueType->toString());
      const Text& name = *I;

      // Verify that the target is a struct
      if (!hueType->isStructure()) {
        return error((std::string("Base of symbol \"") + symbol->toString()
                     + "\" does not refer to a structure"));
      }
      const ast::StructType* hueST = static_cast<const ast::StructType*>(hueType);

      // Find member offset
      size_t memberIndex = hueST->indexOf(name);
      if (memberIndex == SIZE_MAX) {
        return error(std::string("Unknown symbol \"") + name.UTF8String()
                     + "\" in structure " + hueST->toString() );
      }

      // Type for name
      hueType = (*hueST)[name];
      assert(hueType != 0);
      assert(V->getType()->isPointerTy()); // V should be a pointer
      assert(V->getType()->getNumContainedTypes() == 1); // V should reference exactly one item
      assert(V->getType()->getContainedType(0)->isStructTy()); // V[0] should be a struct

      //rlog("V entry: " << llvmTypeToString(V->getType())); V->dump();

      // Get element pointer
      V = builder_.CreateStructGEP(V, memberIndex, name.UTF8String());

      // Struct member is a pointer to something
      assert(V->getType()->isPointerTy() && V->getType()->getNumContainedTypes() == 1);

      // Load the member
      V = builder_.CreateLoad(V, "loadsm");
    }

    return V;
  }

  // Value must be either a global or in the same scope
  assert(blockScope() != 0);
  if (target.owningScope != blockScope()) {
    if (!GlobalValue::classof(target.value)) {
      // TODO: Future: If we support funcs that capture its environment, that code should
      // likely live here.
      return error((std::string("Unknown symbol \"") + symbol->toString() + "\" (no reachable symbols in scope)").c_str());
    }
#if DEBUG_LLVM_VISITOR
    else {
      rlog("Resolving \"" << symbol << "\" to a global constant");
    }
#endif
  } 
#if DEBUG_LLVM_VISITOR
  else {
    if (GlobalValue::classof(target.value)) {
      rlog("Resolving \"" << symbol << "\" to a global & local constant");
    } else {
      rlog("Resolving \"" << symbol << "\" to a local symbol");
    }
  }
#endif

  // Load an alloca or return a reference
  if (target.isAlloca()) {
    return builder_.CreateLoad(target.value, symbol->toString().c_str());
  } else {
    return target.value;
  }
}

#include "_VisitorImplFooter.h"
