// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "_VisitorImplHeader.h"


Value* Visitor::codegenConstantStructure(StructType *ST, const std::vector<Constant*>& fields, const Text& name) {
  Constant* V = ConstantStruct::get(ST, fields);

  std::string irname;
  GlobalValue::LinkageTypes linkage;

  if (name.empty()) {
    irname = "struct";
    linkage = GlobalValue::InternalLinkage;
  } else {
    irname = name.UTF8String();
    linkage = GlobalValue::ExternalLinkage;

    // Expose as global var
    V = new GlobalVariable(*module_, ST, 
      /*isConstant=*/  true,
      /*Linkage=*/     linkage,
      /*Initializer=*/ V,
      /*Name=*/        irname);
    //GV->setAlignment(4);
  }

  Type* PT = PointerType::get(ST, 0);
  return builder_.CreatePointerCast(V, PT);
}


Value* Visitor::codegenDynamicStructure(StructType *ST, const std::vector<Value*>& values, const Text& name) {
  DEBUG_TRACE_LLVM_VISITOR;
  AllocaInst* allocaV = builder_.CreateAlloca(ST, 0, name.UTF8String());
  //Value* V = builder_.CreateLoad(allocaV);

  for (unsigned i = 0, L = values.size(); i != L; ++i) {
    Value *elementV = builder_.CreateStructGEP(allocaV, i);
    builder_.CreateStore(values[i], elementV);
  }

  return allocaV;
}


Value* Visitor::codegenStructure(const ast::Structure *structure, const Text& name) {
  DEBUG_TRACE_LLVM_VISITOR;

  const ast::StructType* astST = static_cast<const ast::StructType*>(structure->resultType());

  std::vector<Value*> fields;
  std::vector<Constant*> constFields;
  const ast::ExpressionList& expressions = structure->block()->expressions();
  bool initializerIsConstant = true;

  for (ast::ExpressionList::const_iterator I = expressions.begin(), E = expressions.end(); I != E; ++I) {
    ast::Expression* expr = *I;
    Value* V = codegen(expr);
    if (V == 0) return 0; // todo cleanup

    if (initializerIsConstant) {
      if (Constant::classof(V)) {
        constFields.push_back(static_cast<Constant*>(V));
      } else {
        initializerIsConstant = false;
      }
    }

    fields.push_back(V);
  }

  StructType *ST = getExplicitStructType(astST);
  if (ST == 0) return 0;

  if (initializerIsConstant) {
    return codegenConstantStructure(ST, constFields, name);
  } else {
    return codegenDynamicStructure(ST, fields, name);
  }
}

#include "_VisitorImplFooter.h"
