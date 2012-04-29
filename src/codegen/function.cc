#include <llvm/Support/CallSite.h>
#include <llvm/Analysis/Verifier.h>

#include "_VisitorImplHeader.h"

Value *Visitor::codegenFunction(const ast::Function *node,
                                std::string name,  // = ""
                                Type* returnType,  // = 0
                                Value* returnValue // = 0
                                ) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  // TODO: Parser or something should infer the return type BEFORE we get here so we
  // can build a proper function interface definition.
  
  // Figure out return type (unless it's been overridden by returnType) if
  // the interface declares the return type.
  if (returnType == 0 && node->interface()->returnCount() != 0) {
    returnType = returnTypeForFunctionInterface(node->interface());
    if (returnType == 0) return error("Unable to transcode return type from AST to IR");
  }
  
  // Function *F = Function::Create(FunctionType::get(builder_.getVoidTy(), false),
  //                                       llvm::Function::AppendingLinkage, "tmpfunc", module_);
  
  // Generate interface
  Function* F = codegenFunctionInterface(node->interface(), name, returnType);
  //Function* F = codegenFunctionInterface(node->interface(), "", returnType);
  if (F == 0) return 0;

  // Setup function body
  BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "", F);
  assert(BB != 0);
  BlockScope bs(*this, BB);
  
  // setSymbol for arguments, and alloca+store if mutable
  ast::VariableList *args = node->interface()->args();
  if (args) {
    unsigned i = 0;
    for (Function::arg_iterator AI = F->arg_begin(); i != args->size(); ++AI, ++i) {
      Argument& arg = *AI;
      ast::Variable* var = (*args)[i];
      
      if (var->isMutable()) {
        // Store mutable value using a STORE instruction (SSA w/ pointers)
      
        // Create an alloca for this variable.
        IRBuilder<> TmpB(&F->getEntryBlock(), F->getEntryBlock().begin());
        AllocaInst *Alloca = TmpB.CreateAlloca(arg.getType(), 0, arg.getName());

        // Store the initial value into the alloca.
        builder_.CreateStore(&arg, Alloca);
      
        // Register or override the value ref with the alloca inst
        bs.setSymbol(var->name(), Alloca, /* isMutable = */true);
      } else {
        // Register the symbol
        bs.setSymbol(var->name(), &arg, /* isMutable = */false);
      }
      
    }
  }
  
  // Add named function to symbol list
  //if (!name.empty()) {
    //block()->
  //}
  
  dumpBlockSymbols();
  
  // Generate block code
  Value *lastValue = codegenBlock(node->body(), BB);
  
  // Failed to generate body?
  if (lastValue == 0) {
    // Error reading body, remove function.
    F->eraseFromParent();
    return 0;
  }
  
  // Set the return value
  if (returnValue == 0) {
    returnValue = lastValue;
  }
  
  // If returnType is nil, that means we should infer the type
  if (returnType == 0) {
    returnType = returnValue->getType();
    //FunctionType *getFunctionType();
  }
  
  // Return value changed -- generate func interface
  if (F->getReturnType()->getTypeID() != returnType->getTypeID()) {
    F->eraseFromParent();
    return error("Not implemented: Function return type inference");
  /*
    // From: http://llvm.org/docs/doxygen/html/DeadArgumentElimination_8cpp_source.html
    
    // Derive new function type with different return type but same parameters
    // TODO: We could use the same code here to update inferred parameter types
    FunctionType *FTy = F->getFunctionType();
    std::vector<Type*> params(FTy->param_begin(), FTy->param_end());
    FunctionType *NFTy = FunctionType::get(returnType, params, false);
    
    // Create the new function body and insert it into the module
    Function *NF = Function::Create(NFTy, F->getLinkage());
    NF->copyAttributesFrom(F);
    F->getParent()->getFunctionList().insert(F, NF);
    NF->takeName(F);
    
    
    // Loop over all of the callers of the function, transforming the call sites
    // to pass in a smaller number of arguments into the new function.
    //
    std::vector<Value*> Args;
    size_t NumArgs = params.size();
    while (!F->use_empty()) {
      std::cerr << "MOS" << std::endl;
      CallSite CS(F->use_back());
      Instruction *Call = CS.getInstruction();

      // Pass all the same arguments.
      Args.assign(CS.arg_begin(), CS.arg_begin() + NumArgs);

      // Drop any attributes that were on the vararg arguments.
      AttrListPtr PAL = CS.getAttributes();
      if (!PAL.isEmpty() && PAL.getSlot(PAL.getNumSlots() - 1).Index > NumArgs) {
        SmallVector<AttributeWithIndex, 8> AttributesVec;
        for (unsigned i = 0; PAL.getSlot(i).Index <= NumArgs; ++i)
          AttributesVec.push_back(PAL.getSlot(i));
        if (Attributes FnAttrs = PAL.getFnAttributes())
          AttributesVec.push_back(AttributeWithIndex::get(~0, FnAttrs));
        PAL = AttrListPtr::get(AttributesVec.begin(), AttributesVec.end());
      }

      Instruction *New;
      if (InvokeInst *II = dyn_cast<InvokeInst>(Call)) {
        New = InvokeInst::Create(NF, II->getNormalDest(), II->getUnwindDest(), Args, "", Call);
        cast<InvokeInst>(New)->setCallingConv(CS.getCallingConv());
        cast<InvokeInst>(New)->setAttributes(PAL);
      } else {
        New = CallInst::Create(NF, Args, "", Call);
        cast<CallInst>(New)->setCallingConv(CS.getCallingConv());
        cast<CallInst>(New)->setAttributes(PAL);
        if (cast<CallInst>(Call)->isTailCall())
          cast<CallInst>(New)->setTailCall();
      }
      New->setDebugLoc(Call->getDebugLoc());

      Args.clear();

      if (!Call->use_empty())
        Call->replaceAllUsesWith(New);

      New->takeName(Call);

      // Finally, remove the old call from the program, reducing the use-count of
      // F.
      Call->eraseFromParent();
    }
    
    
    // Since we have now created the new function, splice the body of the old
    // function right into the new function, leaving the old rotting hulk of the
    // function empty.
    NF->getBasicBlockList().splice(NF->begin(), F->getBasicBlockList());
    
    Function *OF = F;
    F = NF;
    
    // Finally, nuke the old function.
    OF->eraseFromParent();*/
  }
  
  // Check return type against function's declared return type
  if (F->getFunctionType()->isValidReturnType(returnType) == false) {
    F->eraseFromParent();
    
    std::cerr << "F " << name << std::endl;
    std::cerr << "LT: "; if (!returnValue->getType()) std::cerr << "<nil>" << std::endl; else {
      returnValue->getType()->dump(); std::cerr << std::endl; }
    std::cerr << "RT: "; if (!returnType) std::cerr << "<nil>" << std::endl; else {
      returnType->dump(); std::cerr << std::endl; }
    
    return error("Function return type mismatch (does not match declared return type)");
  }
  
  // Finish off the function by defining the return value
  // See CreateAggregateRet for returning multiple values
  // See CreateRetVoid for no return value
  //std::cerr << "builder_.CreateRet ";
  //lastValue->dump();
  ReturnInst *retInst;
  if (returnValue) {
    // Explicit return value
    retInst = builder_.CreateRet(returnValue);
  } else {
    retInst = builder_.CreateRet(lastValue);
  }
  if (!retInst) {
    F->eraseFromParent();
    return error("Failed to build terminating return instruction");
  }
  
  //TerminatorInst *tinst = BB->getTerminator();
  //std::cerr << "BB->getTerminator(): ";
  //if (!tinst) std::cerr << "<nil>" << std::endl; else tinst->dump();

  // Validate the generated code, checking for consistency.
  verifyFunction(*F);

  return F;
}

#include "_VisitorImplFooter.h"
