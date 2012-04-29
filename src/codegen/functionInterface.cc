#include "_VisitorImplHeader.h"

Function *Visitor::codegenFunctionInterface(const ast::FunctionInterface *node,
                                            std::string name, // = "",
                                            Type *returnType  // = 0
                                            ) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  // No return type means void
  if (returnType == 0) {
    returnType = builder_.getVoidTy();
  }
  
  // Make the function type:  double(double,double) etc.
  FunctionType *FT;
  ast::VariableList *args = node->args();
  if (args != 0) {
    std::vector<Type*> floatArgs(args->size(), Type::getDoubleTy(getGlobalContext()));
    FT = FunctionType::get(returnType, floatArgs, /*isVararg = */false);
  } else {
    //std::vector<Type*> noArgs();
    FT = FunctionType::get(returnType, /*isVararg = */false);
  }
  if (!FT) return (Function*)error("Failed to create function type");

  // Create function
  GlobalValue::LinkageTypes linkageType = GlobalValue::PrivateLinkage;
  if (node->isPublic()) {
    linkageType = GlobalValue::ExternalLinkage;
  }
  Function *F = Function::Create(FT, linkageType, name, module_);
  if (!F) return (Function*)error("Failed to create function");
  
  // No unwinding
  F->setDoesNotThrow();

  // If F conflicted, there was already something named 'Name'.  If it has a
  // body, don't allow redefinition or reextern.
  if (F->getName() != name) {
    // Delete the one we just made and get the existing one.
    F->eraseFromParent();
    F = module_->getFunction(name);
  
    // If F already has a body, reject this.
    if (!F->empty()) {
      return (Function*)error("redefinition of a concrete function");
      return 0;
    }
  
    // If F took a different number of args, reject.
    if ( (args == 0 && F->arg_size() != 0) || (args && F->arg_size() != args->size()) ) {
      return (Function*)error("redefinition of a function with different arguments");
    }
  }

  // Set names for all arguments.
  if (args) {
    unsigned i = 0;
    for (Function::arg_iterator AI = F->arg_begin(); i != args->size(); ++AI, ++i) {
      const std::string& argName = (*args)[i]->name();
      AI->setName(argName);
    }
  }

  return F;
}

#include "_VisitorImplFooter.h"
