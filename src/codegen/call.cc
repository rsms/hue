#include "_VisitorImplHeader.h"

// Returns true if V can be used as the target in a call instruction
static FunctionType* functionTypeForValue(Value* V) {
  if (V == 0) return 0;
  
  Type* T = V->getType();
  
  if (T->isFunctionTy()) {
    return static_cast<FunctionType*>(T);
  } else if (   T->isPointerTy()
             && T->getNumContainedTypes() == 1
             && T->getContainedType(0)->isFunctionTy() ) {
    return static_cast<FunctionType*>(T->getContainedType(0));
  } else {
    return 0;
  }
}

// Returns true if V can be used as the target in a call instruction
inline static bool valueIsCallable(Value* V) {
  return !!functionTypeForValue(V);
  //if (V == 0) return 0;
  //
  //Type* T = V->getType();
  //
  //if (T->isFunctionTy()) {
  //  return true;
  //} else if (   T->isPointerTy()
  //           && T->getNumContainedTypes() == 1
  //           && T->getContainedType(0)->isFunctionTy() ) {
  //  return true;
  //} else {
  //  return false;
  //}
}


Value *Visitor::codegenCall(const ast::Call* node) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  // Find value that the symbol references.
  Value* targetV = resolveSymbol(node->calleeName());
  if (targetV == 0) return 0;
  
  // Check type (must be a function) and get FunctionType in one call.
  FunctionType* FT = functionTypeForValue(targetV);
  if (!FT) return error("Trying to call something that is not a function");

  // Local ref to input arguments, for our convenience.
  const ast::Call::ArgumentList& inputArgs = node->arguments();
  
  // Check arguments.
  if (static_cast<size_t>(FT->getNumParams()) != inputArgs.size()) {
    return error("Incorrect number of arguments passed to function call");
  }

  // Build argument list by codegen'ing all input variables
  std::vector<Value*> argValues;
  ast::Call::ArgumentList::const_iterator inIt = inputArgs.begin();
  FunctionType::param_iterator ftIt = FT->param_begin();
  unsigned i = 0;
  for (; inIt < inputArgs.end(); ++inIt, ++ftIt, ++i) {
    // First, codegen the input value
    Value* inputV = codegen(*inIt);
    if (inputV == 0) return 0;
    
    // Verify that the input type matches the expected type
    Type* expectedT = *ftIt;
    Type* inputT = inputV->getType();
    
    if (inputT->getTypeID() != expectedT->getTypeID()) {
      if (inputT->canLosslesslyBitCastTo(expectedT))
        rlogw("TODO: canLosslesslyBitCastTo == true");
      // TODO: Potentially cast simple values like numbers
      return error(R_FMT("Invalid type for argument " << i << " in call to " << node->calleeName()));
    
    } else if (expectedT->isIntegerTy() && expectedT->getPrimitiveSizeInBits() != inputT->getPrimitiveSizeInBits()) {
      // Cast integer
      if (   expectedT->getPrimitiveSizeInBits() < inputT->getPrimitiveSizeInBits()
          && !inputT->canLosslesslyBitCastTo(expectedT)) {
        warning("Implicit truncation of integer");
      }
      
      // This helper function will make either a bit cast, int extension or int truncation
      inputV = builder_.CreateIntCast(inputV, expectedT, /*isSigned = */true);

    } else if (expectedT->isDoubleTy() && expectedT->getPrimitiveSizeInBits() != inputT->getPrimitiveSizeInBits()) {
      // Cast floating point number
      if (inputT->canLosslesslyBitCastTo(expectedT)) {
        inputV = builder_.CreateBitCast(inputV, expectedT, "bcasttmp");
      } else if (expectedT->getPrimitiveSizeInBits() < inputT->getPrimitiveSizeInBits()) {
        warning("Implicit truncation of floating point number");
        inputV = builder_.CreateFPTrunc(inputV, expectedT, "fptrunctmp");
      } else {
        inputV = builder_.CreateFPExt(inputV, expectedT, "fpexttmp");
      }
    }
    
    argValues.push_back(inputV);
  }
  
  // Create call instruction
  //rlog("targetV ->"); targetV->dump();
  return builder_.CreateCall(targetV, argValues, node->calleeName()+"_res");
}

#include "_VisitorImplFooter.h"
