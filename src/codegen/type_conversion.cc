// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "_VisitorImplHeader.h"


StructType* Visitor::getExplicitStructType(const ast::StructType* astST) {
  assert(module_ != 0);
  assert(astST != 0);
  std::string canonicalName = astST->canonicalName();

  StructType *ST = module_->getTypeByName(canonicalName);
  if (ST) return ST;

  std::vector<Type*> irTypes;
  irTypes.reserve(astST->size());

  for (ast::TypeList::const_iterator I = astST->types().begin(),
       E = astST->types().end(); I != E; ++I)
  {
    llvm::Type *T = IRTypeForASTType(*I);
    if (T == 0) return 0;
    irTypes.push_back(T);
  }

  return StructType::create(llvm::getGlobalContext(), makeArrayRef(irTypes),
    canonicalName, astST->isPacked());
}


llvm::FunctionType* Visitor::getLLVMFuncTypeForASTFuncType(const ast::FunctionType* astFT,
                                                           llvm::Type* returnType) {
  // Build argument spec and create the function type:  double(double,double) etc.
  if (returnType == 0) returnType = returnTypeForFunctionType(astFT);
  std::vector<Type*> argSpec;
  if (!IRTypesForASTVariables(argSpec, astFT->args())) return 0;
  return FunctionType::get(returnType, argSpec, /*isVararg = */false);
}


StructType* Visitor::getArrayStructType(Type* elementType) {
  // { length i64, data elementType* }
  StructType *T = arrayStructTypes_[elementType];
  if (T == 0) {
    Type *tv[2];
    tv[0] = Type::getInt64Ty(getGlobalContext()); // i64
    //tv[1] = PointerType::get(Type::getInt8Ty(getGlobalContext()), 0); // i8*
    tv[1] = PointerType::get(elementType, 0); // i8*
    ArrayRef<Type*> elementTypes = makeArrayRef(tv, 2);
    T = StructType::get(getGlobalContext(), elementTypes, /*isPacked = */true);
    arrayStructTypes_[elementType] = T;
  }
  return T;
}


llvm::Type *Visitor::IRTypeForASTType(const ast::Type* T) {
  if (T == 0) return builder_.getVoidTy();
  switch (T->typeID()) {
    case ast::Type::Nil:    return builder_.getInt1Ty();
    case ast::Type::Float:  return builder_.getDoubleTy();
    case ast::Type::Int:    return builder_.getInt64Ty();
    case ast::Type::Char:   return builder_.getInt32Ty();
    case ast::Type::Byte:   return builder_.getInt8Ty();
    case ast::Type::Bool:   return builder_.getInt1Ty();
    
    case ast::Type::Array: {
      llvm::Type* elementType =
        IRTypeForASTType((reinterpret_cast<const ast::ArrayType*>(T))->type());
      //return llvm::PointerType::get(elementType, 0);
      // Return a pointer to a struct of the array type: <{ i64, elementType* }>*
      return llvm::PointerType::get(getArrayStructType(elementType), 0);
    }

    case ast::Type::StructureT: {
      const ast::StructType* astST = static_cast<const ast::StructType*>(T);
      StructType *irST = getExplicitStructType(astST);
      return llvm::PointerType::get(irST, 0);
    }

    case ast::Type::FuncT: {
      const ast::FunctionType* astST = static_cast<const ast::FunctionType*>(T);
      llvm::FunctionType *FT = getLLVMFuncTypeForASTFuncType(astST);
      return llvm::PointerType::get(FT, 0);
    }

    //case ast::Type::Named: ...
    default: {
      error(std::string("No conversion from AST type ") + T->toString() + " to IR type");
      return 0;
    }
  }
}

llvm::ArrayType* Visitor::getArrayTypeFromPointerType(llvm::Type* T) const {
  return (    T->isPointerTy()
           && T->getNumContainedTypes() == 1
           && llvm::ArrayType::classof(T = T->getContainedType(0))
            ? static_cast<llvm::ArrayType*>(T) : 0 );
}

ast::Type::TypeID Visitor::ASTTypeIDForIRType(const llvm::Type* T) {
  switch(T->getTypeID()) {
    case llvm::Type::VoidTyID:      return ast::Type::Nil;
    case llvm::Type::DoubleTyID:    return ast::Type::Float;
    case llvm::Type::IntegerTyID: {
      switch (T->getPrimitiveSizeInBits()) {
        case 1: return ast::Type::Bool;
        case 8: return ast::Type::Byte;
        case 32: return ast::Type::Char;
        case 64: return ast::Type::Int;
        default: return ast::Type::MaxTypeID;
      }
    }
    //case llvm::Type::FunctionTyID:
    // case StructTyID: return       ///< 11: Structures
    // case ArrayTyID: return        ///< 12: Arrays
    // case PointerTyID: return      ///< 13: Pointers
    // case VectorTyID: return       ///< 14: SIMD 'packed' format, or other vector type
    default: return ast::Type::MaxTypeID;
  }
}

ast::Type *Visitor::ASTTypeForIRType(const llvm::StructType* T) {
  return (ast::Type *)error("Not implemented: Visitor::ASTTypeForIRType(llvm::StructType)");
}

ast::Type *Visitor::ASTTypeForIRType(const llvm::Type* T) {

  if (T->isPointerTy() && T->getContainedType(0)->isStructTy()) {
    llvm::StructType* ST = static_cast<llvm::StructType*>(T->getContainedType(0));
    return ASTTypeForIRType(ST);
  } else {
    ast::Type::TypeID typeID = ASTTypeIDForIRType(T);
    return typeID == ast::Type::MaxTypeID ? 0 : new ast::Type(typeID);
  }
}


bool Visitor::IRTypesForASTVariables(std::vector<Type*>& argSpec, ast::VariableList *argVars) {
  if (argVars != 0 && argVars->size() != 0) {
    argSpec.reserve(argVars->size());
    ast::VariableList::const_iterator it = argVars->begin();
    
    for (; it != argVars->end(); ++it) {
      ast::Variable* var = *it;
      
      // Type should be inferred?
      if (var->hasUnknownType()) {
        // TODO: runtime type inference support.
        return !!error("NOT IMPLEMENTED: runtime type inference of func arguments");
      }
      
      // Lookup IR type for AST type
      Type* T = IRTypeForASTType(var->type());
      if (T == 0)
        return !!error(R_FMT("No conversion for AST type " << var->toString() << " to IR type"));
      
      // Use T
      argSpec.push_back(T);
    }
  }
  
  return true;
}


#include "_VisitorImplFooter.h"
