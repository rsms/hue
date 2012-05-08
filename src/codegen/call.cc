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
    
    // Cast input value to argument type if needed
    Type* expectedT = *ftIt;
    inputV = castValueTo(inputV, expectedT);
    if (inputV == 0)
      return error(R_FMT("Invalid type for argument " << i << " in call to " << node->calleeName()));
    
    argValues.push_back(inputV);
  }
  
  // Create call instruction
  if (FT->getReturnType()->isVoidTy()) {
    builder_.CreateCall(targetV, argValues);
    return ConstantInt::getFalse(getGlobalContext());
  } else {
    return builder_.CreateCall(targetV, argValues, node->calleeName().UTF8String() + "_res");
  }
}

#include "_VisitorImplFooter.h"
