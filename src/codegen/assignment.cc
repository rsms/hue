// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#include <llvm/Instructions.h>

#include "../Mangle.h"
#include "_VisitorImplHeader.h"

Value* Visitor::createNewLocalSymbol(ast::Variable *variable, Value *rhsValue,
                                     bool warnRedundantTypeDecl)
{
  Type* rhsOriginalType = rhsValue->getType();
  Value* V = rhsValue;
  const bool storageTypeIsKnown = variable->hasUnknownType() == false;
  Type *varT = 0;
  
  // Map Variable's type
  if (storageTypeIsKnown) {
    varT = IRTypeForASTType(variable->type());
    if (varT == 0) return error("No encoding for variable's AST type");
  }
  
  //bool requireAlloca = false;
  
  // If storage type is explicit/known --and-- RHS is a primitive: cast if needed
  if (storageTypeIsKnown && rhsOriginalType->isPrimitiveType()) {
    // If storage type is different than value, attempt to convert the value
    if (varT->getTypeID() != V->getType()->getTypeID()) {
      //requireAlloca = true; // Require an alloca since we are replacing V with a cast instr
      V = castValueTo(V, varT);
      if (V == 0) return error("Symbol/variable type is different than value type");
    }
  }
  
  // Alloca+STORE if RHS is not a primitive --or-- the var is mutable
  if (V != rhsValue || variable->isMutable()) {
    rlog("Alloca+STORE variable \"" << variable->name() << "\"");
    V = createAllocaAndStoreValue(V, variable->name());
    // Note: alloca optimization passes will take care of removing any unneeded
    // allocas that are the result of constant inlining. E.g.
    //   x = 3.4
    //   y = 3
    //   z Int = y + x
    //  
    // Would result in the following IR:
    //
    //   %z = alloca i64
    //   store i64 6, i64* %z
    //
    // Which, after an alloca optimization pass, will simply reference the constant i64 6:
    //
    //   %z = i64 6
    //
  } else {
    Type* valueT = V->getType();
    
    if (storageTypeIsKnown) {
      
      if (warnRedundantTypeDecl) {
        warning(std::string("Unneccessary type declaration of constant variable '")
                + variable->name().UTF8String() + "'");
      }
    
      // Check types
      if (varT != valueT) {
        if (ArrayType::classof(varT) && ArrayType::classof(valueT)) {
          Type* varElementT = static_cast<ArrayType*>(varT)->getElementType();
          Type* valueElementT = static_cast<ArrayType*>(valueT)->getElementType();
          if (varElementT != valueElementT) {
            return error("Different types of arrays");
          }
          // else:
          // Both are same kind of arrays, but of different size. Since we don't support type
          // definitions with fixes array size, this one is easy: valueT wins. We do nothing.
        } else {
          // Yikes. Type mis-match. This could be corrected by the programmer by having
          // the type being inferred instead of explicitly defined, since the var is constant.
          return error("Type mismatch between variable and value");
        }
      }
    }
    
    // if (T->isPointerTy() && T->getNumContainedTypes() == 1 && T->getContainedType(0)->isFunctionTy()) {
    //   rlog("is a pointer to a func");
    // }
  }
  
  blockScope()->setSymbol(variable->name(), V, variable->isMutable());
  
  return V;
}


// Assignment
Value *Visitor::codegenAssignment(const ast::Assignment* node) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  // TODO: Make assignments that occur in the module block
  // global. That way we can also set their linkage to external
  // in order to have them exported, if marked for export. Use GlobalValue
  // for constant symbols and GlobalVariable for variables.
  
  // Get the list of variables
  const ast::VariableList *variables = node->variables();
  assert(variables->size() == 1); // TODO: support >1 LHS variable

  // Codegen the RHS.
  Value *rhsValue;
  ast::FunctionType* hueFuncType = 0;
  
  if (node->rhs()->nodeTypeID() == ast::Node::TFunction) {
    const ast::Function* funcNode = static_cast<const ast::Function*>(node->rhs());
    hueFuncType = funcNode->functionType();
    
    // Generate a globally unique mangled name
    //std::string mangledName = uniqueMangledName((*variables)[0]->name());
    std::string mangledName = module_->getModuleIdentifier() + ":";
    mangledName += (*variables)[0]->name().UTF8String();
    mangledName += mangle(*funcNode->functionType());
    
    // Has this function already been declared? (that is, same namespace, name, arg types and result types)
    if (module_->getNamedValue(mangledName) != 0 || module_->getNamedGlobal(mangledName) != 0) {
      return error("Implementation has already been defined for the symbol");
    }
    
    // Generate function
    rhsValue = codegenFunction(funcNode, mangledName);

  } else if (node->rhs()->nodeTypeID() == ast::Node::TExternalFunction) {
    const ast::ExternalFunction* externalFuncNode = static_cast<const ast::ExternalFunction*>(node->rhs());
    hueFuncType = externalFuncNode->functionType();
    rhsValue = codegenExternalFunction(externalFuncNode);

  } else if (node->rhs()->nodeTypeID() == ast::Node::TCall) {
    // Create type list from LHS variables, which the call codegen can use in the case
    // where there are multiple function candidates that only differ on return type.
    ast::TypeList types;
    ast::Variable::typeListFromVariableList(types, *variables);
    
    rhsValue = codegenCall(static_cast<const ast::Call*>(node->rhs()), &types);
    
  } else {
    rhsValue = codegen(node->rhs());
  }
  if (rhsValue == 0) return 0;
  
  
  // For each variable
  ast::VariableList::const_iterator it = variables->begin();
  for (; it != variables->end(); ++it) {
    ast::Variable *variable = (*it);
    
    // Functions are treated differently
    FunctionType* FT = functionTypeForValue(rhsValue);
    if (FT) {
      assert(hueFuncType != 0);
      if (blockScope()->setFunctionSymbol(variable->name(), hueFuncType, FT, rhsValue) == false) {
        return error("Implementation has already been defined for the symbol");
      }

    } else {
      // We are dealing with something not a function
  
      // Look up the name in this direct scope.
      const Symbol& symbol = blockScope()->lookupSymbol(variable->name());
      Value *existingValue = symbol.value;
      
      // Check if there's a function symbol with the same name
      if (existingValue == 0) {
        if (blockScope()->lookupFunctionSymbols(variable->name()) != 0) {
          return error("Symbol has already been defined");
        }
      }
    
      if (existingValue == 0) {
        // New symbol
        bool warnRedundantTypeDecl = (node->rhs()->nodeTypeID() != ast::Node::TCall);
        rhsValue = createNewLocalSymbol(variable, rhsValue, warnRedundantTypeDecl);
        if (rhsValue == 0) return 0;

      } else {
        // Existing symbol
        if (symbol.isMutable) {
          if (!AllocaInst::classof(existingValue)) {
            return error("not implemented: promote mutatable ref to alloca");
          } else {
            // STORE the value
            llvm::StoreInst* si = builder_.CreateStore(rhsValue, static_cast<AllocaInst*>(existingValue));
            rhsValue = si;
          }
        } else {
          rlog(variable->name() << " is not mutable");
          return error("Symbol has already been defined");
        }
      }
    
    }
  
  }

  return rhsValue;
}

#include "_VisitorImplFooter.h"
