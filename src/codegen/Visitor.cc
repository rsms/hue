#include "_VisitorImplHeader.h"

void Visitor::dumpBlockSymbols() {
  if (blockStack_.empty()) return;
  const char *indent = "                                                                        ";
  BlockStack::const_iterator it = blockStack_.begin();
  int L = 1;
  std::cerr << "{" << std::endl;
  
  for (; it != blockStack_.end(); ++it, ++L) {
    BlockScope *bs = *it;
    BasicBlock *bb = bs->block();
    std::string ind(indent, std::min<int>(L*2, strlen(indent)));
    llvm::ValueSymbolTable* symbols = bb->getValueSymbolTable();
    llvm::ValueSymbolTable::const_iterator it2 = symbols->begin();
    
    for (; it2 != symbols->end(); ++it2) {
      std::cerr << ind << it2->getKey().str() << ": ";
    
      llvm::Value *V = it2->getValue();
      llvm::Type *T = V->getType();
      if (T->isLabelTy()) {
        std::cerr << "label" << std::endl;
      } else {
        if (T->isPointerTy()) std::cerr << "pointer ";
        V->dump();
      }
    }
    
    if (L-1 != blockStack_.size()-1) {
      std::cerr << ind << "{" << std::endl;
    }
  }
  
  while (--L) {
    std::string ind(indent, std::min<int>((L-1)*2, strlen(indent)));
    std::cerr << ind << "}" << std::endl;
  }
}


// Returns a module-global uniqe mangled name rooted in *name*
std::string Visitor::uniqueMangledName(const std::string& name) {
  std::string mname;
  int i = 0;
  while (1) {
    // Take the LHS var name and use it for the function name
    if (i > 0) {
      char buf[12]; snprintf(buf, 12, "$$%d", i);
      mname = mangledName(name + buf);
    } else {
      mname = mangledName(name);
    }
    if (module_->getNamedValue(mname) == 0 && module_->getNamedGlobal(mname) == 0) {
      // We got a unique name
      break;
    }
    ++i;
  }
  return mname;
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
      Type* T = IRTypeForASTTypeDecl(*var->type());
      if (T == 0)
        return !!error(R_FMT("No conversion for AST type " << var->toString() << " to IR type"));
      
      // Use T
      argSpec.push_back(T);
    }
  }
  
  return true;
}


// ----------- trivial generators ------------


llvm::Module *Visitor::genModule(llvm::LLVMContext& context, std::string moduleName, const ast::Function *root) {
  DEBUG_TRACE_LLVM_VISITOR;
  llvm::Module *module = new Module(moduleName, context);
  
  module_ = module;
  
  llvm::Value *returnValue = llvm::ConstantInt::get(llvm::getGlobalContext(), APInt(64, 0, true));
  //llvm::Value *moduleFunc = codegenFunction(root, std::string("minit__") + moduleName, builder_.getInt64Ty());
  llvm::Value *moduleFunc = codegenFunction(root, "main", returnValue->getType(), returnValue);
  
  module_ = 0;
  
  if (moduleFunc == 0) {
    delete module;
    module = 0;
  }
  
  return module;
}

// ExternalFunction
Value *Visitor::codegenExternalFunction(const ast::ExternalFunction* node) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  // Figure out return type (unless it's been overridden by returnType) if
  // the interface declares the return type.
  Type* returnType = returnTypeForFunctionInterface(node->interface());
  if (returnType == 0) return error("Unable to transcode return type from AST to IR");
  
  return codegenFunctionInterface(node->interface(), node->name(), returnType);
}

// Block
Value *Visitor::codegenBlock(const ast::Block *block, llvm::BasicBlock *BB) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  const ast::Block::NodeList& nodes = block->nodes();
  if (nodes.size() == 0) {
    return error("Empty function implementation when expecting one or more expressions or statements");
  }
  
  ast::Block::NodeList::const_iterator it1 = nodes.begin();
  Value *lastValue = 0;
  for (; it1 < nodes.end(); it1++) {
    
    // xxx
    // if ((*it1)->type == Node::TAssignment && (
    //              ((Assignment*)(*it1))->rhs()->type == Node::TIntLiteral
    //           || ((Assignment*)(*it1))->rhs()->type == Node::TFloatLiteral
    //           || ((Assignment*)(*it1))->rhs()->type == Node::TBinaryOp
    //           || ((Assignment*)(*it1))->rhs()->type == Node::TExternalFunction
    //         )) {
      lastValue = codegen(*it1);
      if (lastValue == 0) return 0;
    //}
  }
  
  return lastValue;
}

// Int
Value *Visitor::codegenIntLiteral(const ast::IntLiteral *intLiteral, bool fixedSize) {
  DEBUG_TRACE_LLVM_VISITOR;
  dumpBlockSymbols();
  // TODO: Infer the minimal size needed if fixedSize is false
  const unsigned numBits = 64;
  return ConstantInt::get(getGlobalContext(), APInt(numBits, intLiteral->text(), intLiteral->radix()));
}

// Float
Value *Visitor::codegenFloatLiteral(const ast::FloatLiteral *floatLiteral, bool fixedSize) {
  DEBUG_TRACE_LLVM_VISITOR;
  // TODO: Infer the minimal size needed if fixedSize is false
  const fltSemantics& size = APFloat::IEEEdouble;
  return ConstantFP::get(getGlobalContext(), APFloat(size, floatLiteral->text()));
}

// Reference or load a symbol
Value *Visitor::resolveSymbol(const std::string& name) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  // Lookup symbol
  const Symbol& symbol = lookupSymbol(name);
  if (symbol.empty())
    return error((std::string("Unknown variable \"") + name + "\"").c_str());
  
  // Value must be either a global or in the same scope
  assert(blockScope() != 0);
  if (symbol.owningScope != blockScope()) {
    if (!GlobalValue::classof(symbol.value)) {
      // TODO: Future: If we support funcs that capture its environment, that code should
      // likely live here.
      return error((std::string("Unknown variable \"") + name + "\" (no reachable symbols in scope)").c_str());
    } else {
      rlog("Resolving \"" << name << "\" to a global constant");
    }
  } else {
    if (GlobalValue::classof(symbol.value)) {
      rlog("Resolving \"" << name << "\" to a global & local constant");
    } else {
      rlog("Resolving \"" << name << "\" to a local variable");
    }
  }

  // Load an alloca or return a reference
  if (symbol.isAlloca()) {
    return builder_.CreateLoad(symbol.value, name.c_str());
  } else {
    return symbol.value;
  }
}


Value *Visitor::codegenSymbol(const ast::Symbol *symbolExpr) {
  DEBUG_TRACE_LLVM_VISITOR;
  assert(symbolExpr != 0);
  return resolveSymbol(symbolExpr->name());
}



Visitor::Symbol Visitor::Symbol::Empty;

#include "_VisitorImplFooter.h"
