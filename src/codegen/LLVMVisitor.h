#ifndef RSMS_CODEGEN_LLVM_VISITOR_H
#define RSMS_CODEGEN_LLVM_VISITOR_H

#define DEBUG_LLVM_VISITOR 1
#if DEBUG_LLVM_VISITOR
  #include "../DebugTrace.h"
  #define DEBUG_TRACE_LLVM_VISITOR DEBUG_TRACE
#else
  #define DEBUG_TRACE_LLVM_VISITOR do{}while(0)
#endif

#include "../ast/Node.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/ValueSymbolTable.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>
#include <llvm/Support/CallSite.h>

#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <deque>

namespace rsms { namespace codegen {
using namespace llvm;
using llvm::Function;
using llvm::FunctionType;
using llvm::Argument;
using llvm::Type;

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for variables etc.
static AllocaInst *CreateEntryBlockAlloca(BasicBlock *basicBlock, Type *type, const std::string &variableName) {
  IRBuilder<> TmpB(basicBlock, basicBlock->begin());
  // AllocaInst - an instruction to allocate memory on the stack
  return TmpB.CreateAlloca(type, 0, variableName.c_str());
}
class LLVMVisitor {
  class BlockScope;
  
  class Symbol {
  public:
    Value *value;
    bool isMutable;
    bool isAlloca() const { return value ? AllocaInst::classof(value) : false; };
    bool empty() const { return value == 0; }
    Symbol() : value(0), isMutable(true) {}
    Symbol(Value *V, bool M = true) : value(V), isMutable(M) {}
  };
  
  typedef std::deque<BlockScope*> BlockStack;
  
  class BlockScope {
    typedef std::map<std::string,Symbol> SymbolMap;
    Symbol EmptySymbol;
  public:
    BlockScope(LLVMVisitor& visitor, BasicBlock *block) : visitor_(visitor), block_(block) {
      visitor_.blockStack_.push_back(this);
      visitor_.builder_.SetInsertPoint(block);
    }
    ~BlockScope() {
      visitor_.blockStack_.pop_back();
      if (visitor_.blockStack_.empty()) {
        visitor_.builder_.ClearInsertionPoint();
      } else {
        visitor_.builder_.SetInsertPoint(visitor_.blockStack_.back()->block());
      }
    }
    
    BasicBlock *block() const { return block_; }
    
    void setSymbol(const std::string& name, Value *V, bool isMutable = true) {
      Symbol& symbol = symbols_[name];
      symbol.value = V;
      symbol.isMutable = isMutable;
    }
    
    const Symbol& lookupSymbol(const std::string& name, bool deep = true) const {
      SymbolMap::const_iterator it = symbols_.find(name);
      if (it != symbols_.end()) {
        return it->second;
      } else {
        // TODO: deep
        return EmptySymbol;
      }
      
      // ValueSymbolTable* symbols = block_->getValueSymbolTable();
      // Value* V = symbols->lookup(name);
      //     
      // if (V == 0 && deep) {
      //   // TODO: dig into parent blocks
      //   std::cerr << "NOT IMPLEMENTED: deep references to symbols" << std::endl;
      //   return 0;
      // } else if (V) {
      //   Type *T = V->getType();
      //   // Do not return references for certain values, like labels
      //   if (T->isLabelTy()) {
      //     V = 0;
      //   }
      // }
      // 
      // std::cerr << "symbol lookup '" << name << "' -> ";
      // if (V) V->dump();
      // else std::cerr << "<nil>" << std::endl;
      // 
      // return V;
    }
    
  private:
    LLVMVisitor& visitor_;
    BasicBlock *block_;
    SymbolMap symbols_;
  };
  
public:
  LLVMVisitor() : module_(NULL), builder_(getGlobalContext()) {}
  
  // Register an error
  Value *error(const char *str) {
    std::ostringstream ss;
    ss << str;
    errors_.push_back(ss.str());
    std::cerr << "\e[31;1m" << ss.str() << "\e[0m" << std::endl;
    return 0;
  }
  
  // Register a warning
  Value *warning(const char *str) {
    std::ostringstream ss;
    ss << str;
    warnings_.push_back(ss.str());
    std::cerr << "\e[35;1mWarning:\e[0m " << ss.str() << std::endl;
    return 0;
  }
  
  const std::vector<std::string>& errors() const { return errors_; };
  const std::vector<std::string>& warnings() const { return warnings_; };
  
  // ---------------------------------------------
  
  // Generate code for a module rooting at *root*
  llvm::Module *genModule(llvm::LLVMContext& context, std::string moduleName, const ast::Function *root) {
    DEBUG_TRACE_LLVM_VISITOR;
    llvm::Module *module = new Module(moduleName, context);
    
    module_ = module;
    
    Value *returnValue = ConstantInt::get(getGlobalContext(), APInt(64, 0, true));
    //Value *moduleFunc = codegenFunction(root, std::string("minit__") + moduleName, builder_.getInt64Ty());
    Value *moduleFunc = codegenFunction(root, "main", returnValue->getType(), returnValue);
    
    module_ = 0;
    
    if (moduleFunc == 0) {
      delete module;
      module = 0;
    }
    
    return module;
  }
  
  
protected:
  // Current block scope, or 0 if none
  inline BlockScope* blockScope() const {
    return blockStack_.empty() ? 0 : blockStack_.back();
  }
  
  // Current block, or 0 if none
  inline BasicBlock* block() const {
    return builder_.GetInsertBlock();
  }
  
  // Dump all symbols in the current block stack
  void dumpBlockSymbols() {
    if (blockStack_.empty()) return;
    const char *indent = "                                                                        ";
    BlockStack::const_iterator it = blockStack_.begin();
    int L = 1;
    std::cerr << "{" << std::endl;
    
    for (; it != blockStack_.end(); ++it, ++L) {
      BlockScope *bs = *it;
      BasicBlock *bb = bs->block();
      std::string ind(indent, std::min<int>(L*2, strlen(indent)));
      ValueSymbolTable* symbols = bb->getValueSymbolTable();
      ValueSymbolTable::const_iterator it2 = symbols->begin();
      for (; it2 != symbols->end(); ++it2) {
        std::cerr << ind << it2->getKey().str() << ": ";
      
        Value *V = it2->getValue();
        Type *T = V->getType();
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
  
  
  Type* returnTypeForFunctionInterface(const ast::FunctionInterface *node) {
    // Find out the return type of the function
    TypeDeclarationList *returnTypes = node->returnTypes();
    if (returnTypes == 0 || returnTypes->size() == 0 /*TODO: || size==1 && type==TNull */) {
      return builder_.getVoidTy();
    } else {
      assert(returnTypes->size() == 1); // TODO: Support multiple return values
      ast::TypeDeclaration *astType = (*returnTypes)[0];
      switch (astType->type) {
        case ast::TypeDeclaration::Int: return builder_.getInt64Ty();
        case ast::TypeDeclaration::Float: return builder_.getDoubleTy();
        //case ast::TypeDeclaration::Func: return builder_.getDoubleTy();
        //case ast::TypeDeclaration::Named: return 1;
        default: return 0;
      }
    }
  }
  
  // Value* lookupSymbol(const std::string& name, bool deep = true) {
  //   if (!blockScope()) return 0;
  //   return blockScope()->lookupSymbol(name, deep);
  // }
  
  
  Type *IRTypeForASTTypeDecl(const ast::TypeDeclaration& typeDecl) {
    switch (typeDecl.type) {
      case ast::TypeDeclaration::Int: return builder_.getInt64Ty();
      case ast::TypeDeclaration::Float: return builder_.getDoubleTy();
      // case ast::TypeDeclaration::Named: -- Custom type
      default: return 0;
    }
  }
  
  // ------------------------------------------------
  
  // Emit LLVM IR for this AST node along with all the things it depends on.
  // "Value" is the class used to represent a
  // "Static Single Assignment (SSA) register" or "SSA value" in LLVM.
  Value *codegen(const ast::Node *node) {
    DEBUG_TRACE_LLVM_VISITOR;
    //std::cout << "Node [" << node->type << "]" << std::endl;
    switch (node->type) {
      case Node::TSymbolExpression: return codegenSymbolExpression((ast::SymbolExpression*)node);
      case Node::TBinaryExpression: return codegenBinaryExpression((ast::BinaryExpression*)node);
      case Node::TFunction: return codegenFunction((ast::Function*)node);
      case Node::TIntLiteralExpression: return codegenIntLiteral((IntLiteralExpression*)node);
      case Node::TFloatLiteralExpression: return codegenFloatLiteral((FloatLiteralExpression*)node);
      case Node::TAssignmentExpression: return codegenAssignment((AssignmentExpression*)node);
      case Node::TExternalFunction: return codegenExternalFunction((ast::ExternalFunction*)node);
      case Node::TCallExpression: return codegenCallExpression((ast::CallExpression*)node);
      default: return error("Unable to generate code for node");
    }
  }
  
  
  // FunctionInterface
  llvm::Function *codegenFunctionInterface(const ast::FunctionInterface *node,
                                           std::string name = "",
                                           Type *returnType = 0) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    // No return type means void
    if (returnType == 0) {
      returnType = builder_.getVoidTy();
    }
    
    // Make the function type:  double(double,double) etc.
    FunctionType *FT;
    VariableList *args = node->args();
    if (args != 0) {
      std::vector<llvm::Type*> floatArgs(args->size(), Type::getDoubleTy(getGlobalContext()));
      FT = FunctionType::get(returnType, floatArgs, /*isVararg = */false);
    } else {
      //std::vector<llvm::Type*> noArgs();
      FT = FunctionType::get(returnType, /*isVararg = */false);
    }
    if (!FT) return (Function*)error("Failed to create function type");
  
    // Create function
    GlobalValue::LinkageTypes linkageType = GlobalValue::PrivateLinkage;
    if (node->isPublic()) {
      linkageType = GlobalValue::ExternalLinkage;
    }
    llvm::Function *F = Function::Create(FT, linkageType, name, module_);
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
      for (llvm::Function::arg_iterator AI = F->arg_begin(); i != args->size(); ++AI, ++i) {
        const std::string& argName = (*args)[i]->name();
        AI->setName(argName);
      }
    }
  
    return F;
  }
  
  
  // ExternalFunction
  Value *codegenExternalFunction(const ast::ExternalFunction* external, std::string name = "") {
    DEBUG_TRACE_LLVM_VISITOR;
    if (name.empty()) {
      return codegenFunctionInterface(external->interface(), external->name());
    } else {
      return codegenFunctionInterface(external->interface(), name);
    }
  }
  
  
  // Function
  Value *codegenFunction(const ast::Function *node,
                         std::string name = "",
                         Type* returnType = 0,
                         Value* returnValue = 0) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    // TODO: Parser or something should infer the return type BEFORE we get here so we
    // can build a proper function interface definition.
    
    // Figure out return type (unless it's been overridden by returnType) if
    // the interface declares the return type.
    if (returnType == 0 && node->interface()->declaresReturnTypes()) {
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
    
    // Alloca + STORE for arguments
    VariableList *args = node->interface()->args();
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
  
  
  // Block
  Value *codegenBlock(const ast::Block *block, BasicBlock *BB) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    const Block::NodeList& nodes = block->nodes();
    if (nodes.size() == 0) {
      return error("Empty function implementation when expecting one or more expressions or statements");
    }
    
    Block::NodeList::const_iterator it1 = nodes.begin();
    Value *lastValue = 0;
    for (; it1 < nodes.end(); it1++) {
      
      // xxx
      // if ((*it1)->type == Node::TAssignmentExpression && (
      //              ((AssignmentExpression*)(*it1))->rhs()->type == Node::TIntLiteralExpression
      //           || ((AssignmentExpression*)(*it1))->rhs()->type == Node::TFloatLiteralExpression
      //           || ((AssignmentExpression*)(*it1))->rhs()->type == Node::TBinaryExpression
      //           || ((AssignmentExpression*)(*it1))->rhs()->type == Node::TExternalFunction
      //         )) {
        lastValue = codegen(*it1);
        if (lastValue == 0) return 0;
      //}
    }
    
    return lastValue;
  }
  
  
  // AssignmentExpression
  Value *codegenAssignment(const ast::AssignmentExpression* node) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    // Get the list of variables
    const VariableList *variables = node->variables();
    assert(variables->size() == 1); // TODO: support >1 LHS variable
    
    // Codegen the RHS.
    Value *rhsValue;
    if (node->rhs()->type == Node::TFunction) {
      // Take the LHS var name and use it for the function name
      if ((*variables)[0]->isMutable() == false) {
        // Don't bother making a poiner to the function, but just gen the function
        return codegenFunction((ast::Function*)node->rhs(), (*variables)[0]->name());
      } else {
        rhsValue = codegenFunction((ast::Function*)node->rhs(), (*variables)[0]->name());
      }
    } else if (node->rhs()->type == Node::TExternalFunction) {
      // Take the LHS var name and use it for the function name
      ast::ExternalFunction* efunc = (ast::ExternalFunction*)node->rhs();
      std::string name;
      if (efunc->name().empty()) {
        name = (*variables)[0]->name();
      }
      if ((*variables)[0]->isMutable() == false) {
        // Don't bother making a poiner to the function, but just gen the external
        return codegenExternalFunction(efunc, name);
      } else {
        rhsValue = codegenExternalFunction(efunc, name);
      }
    } else {
      rhsValue = codegen(node->rhs());
    }
    if (rhsValue == 0) return 0;
    
    // For each variable
    VariableList::const_iterator it = variables->begin();
    for (; it < variables->end(); it++) {
      Variable *variable = (*it);
    
      // Look up the name in this direct scope.
      const Symbol& symbol = blockScope()->lookupSymbol(variable->name(), false);
      Value *variableValue = symbol.value;
      
      if (variableValue == 0) {
        // New local variable
        
        Type *allocaType = 0;
        bool didCast = false;
        
        if (variable->hasUnknownType() == false) {
          allocaType = IRTypeForASTTypeDecl(*variable->type());
          if (allocaType == 0) {
            return error("No encoding for AST type");
          }
          
          if (allocaType->getTypeID() != rhsValue->getType()->getTypeID()) {
            // RHS type is different that storage type
            Type* ST = allocaType;
            Type* VT = rhsValue->getType();
            didCast = true;
            
            // Numbers: cast to storage
            if (ST->isDoubleTy() && VT->isIntegerTy()) { // Promote Int constant to Float
              //warning("Implicit conversion of integer value to floating point storage");
              rhsValue = builder_.CreateSIToFP(rhsValue, ST, "casttmp");
            } else if (ST->isIntegerTy() && VT->isDoubleTy()) { // Demote Float constant to Int
              warning("Implicit conversion of floating point value to integer storage");
              rhsValue = builder_.CreateFPToSI(rhsValue, ST, "casttmp");
            } else {
              return error("Symbol/variable storage type is different than value type");
            }
          }
        } else {
          // Infer type from RHS
          allocaType = rhsValue->getType();
        }
        
        Value *V = rhsValue;
        bool isMutable = variable->isMutable();
        
        if (isMutable || didCast) {
          V = CreateEntryBlockAlloca(&(*block()), allocaType, variable->name());
          builder_.CreateStore(rhsValue, V);
        }
        
        blockScope()->setSymbol(variable->name(), V, variable->isMutable());
        
      } else if (variable->isMutable() && symbol.isMutable) {
        return error("not implemented: value mutation");
      } else {
        return error("trying to mutate an immutable value");
      }
    
    }

    return rhsValue;
  }
  
  
  // CallExpression
  Value *codegenCallExpression(const ast::CallExpression* node) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    // Find the function
    Function *calleeF = module_->getFunction(node->calleeName());
    if (calleeF == 0) {
      const Symbol& symbol = blockScope()->lookupSymbol(node->calleeName());
      Value *alias = symbol.value;
      Type *aliasT = alias ? alias->getType() : 0;
      
      if (aliasT && aliasT->isFunctionTy()) {
        calleeF = (Function*)alias;
      } else if (aliasT && aliasT->isPointerTy() && aliasT->getNumContainedTypes() == 1) {
        Type *containedAliasT = aliasT->getContainedType(0);
        if (containedAliasT->isFunctionTy()) {
          return error("Not implemented: load referenced function from pointer");
        } else {
          return error("Trying to invoke something that is not a function");
        }
      } else {
        //std::cerr << std::endl;
        //aliasT->dump();
        //std::cerr << std::endl;
        //std::cerr << "isFunctionTy(): " << aliasT->isFunctionTy() << std::endl;
        //std::cerr << "isStructTy(): " << aliasT->isStructTy() << std::endl;
        //std::cerr << "isPointerTy(): " << aliasT->isPointerTy() << std::endl;
        //std::cerr << "isDerivedType(): " << aliasT->isDerivedType() << std::endl;
        //std::cerr << "isFirstClassType(): " << aliasT->isFirstClassType() << std::endl;
        //std::cerr << "getNumContainedTypes(): " << aliasT->getNumContainedTypes() << std::endl;
        return error((std::string("Unknown function referenced: ") + node->calleeName()).c_str());
      }
    }
  
    const ast::CallExpression::ArgumentList& args = node->arguments();
  
    // If argument mismatch error.
    if (calleeF->arg_size() != args.size()) {
      std::cerr << "calleeF->arg_size(): " << calleeF->arg_size()
        << ", args.size(): " << args.size() << std::endl;
      return error("Incorrect number of arguments passed to function call");
    }

    std::vector<Value*> argValues;
    ast::CallExpression::ArgumentList::const_iterator it = args.begin();
    for (; it < args.end(); it++) {
      Value *argV = codegen(*it);
      if (argV == 0) return 0;
      argValues.push_back(argV);
    }
  
    return builder_.CreateCall(calleeF, argValues, node->calleeName()+"_res");
  }
  
  
  // Int
  Value *codegenIntLiteral(const ast::IntLiteralExpression *intLiteral, bool fixedSize = true) {
    DEBUG_TRACE_LLVM_VISITOR;
    dumpBlockSymbols();
    // TODO: Infer the minimal size needed if fixedSize is false
    const unsigned numBits = 64;
    return ConstantInt::get(getGlobalContext(), APInt(numBits, intLiteral->text(), intLiteral->radix()));
  }
  
  
  // Float
  Value *codegenFloatLiteral(const ast::FloatLiteralExpression *floatLiteral, bool fixedSize = true) {
    DEBUG_TRACE_LLVM_VISITOR;
    // TODO: Infer the minimal size needed if fixedSize is false
    const fltSemantics& size = APFloat::IEEEdouble;
    return ConstantFP::get(getGlobalContext(), APFloat(size, floatLiteral->text()));
  }
  
  
  // BinaryExpression
  Value *codegenBinaryExpression(BinaryExpression *binExpr) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    Expression *lhs = binExpr->lhs();
    Expression *rhs = binExpr->rhs();
    
    Value *L = codegen(lhs); if (L == 0) return 0;
    Value *R = codegen(rhs); if (R == 0) return 0;
    
    Type* LT = L->getType();
    Type* RT = R->getType();
    Type* type = LT;
    
    // Numbers: cast to richest type if needed
    if (LT->isIntegerTy() && RT->isDoubleTy()) {
      // Cast L to double
      L = builder_.CreateSIToFP(L, (type = RT), "itoftmp");
    } else if (LT->isDoubleTy() && RT->isIntegerTy()) {
      // Cast R to double
      R = builder_.CreateSIToFP(R, (type = LT), "itoftmp");
    }
  
    // std::cerr << "L: "; if (!L) std::cerr << "<nil>" << std::endl; else L->dump();
    // std::cerr << "R: "; if (!R) std::cerr << "<nil>" << std::endl; else R->dump();
    // 
    // LT = L->getType();
    // RT = R->getType();
    // std::cerr << "LT: "; if (!LT) std::cerr << "<nil>" << std::endl; else {
    //   LT->dump(); std::cerr << std::endl; }
    // std::cerr << "RT: "; if (!RT) std::cerr << "<nil>" << std::endl; else {
    //   RT->dump(); std::cerr << std::endl; }
  
    switch (binExpr->operatorValue()) {
      
      case '+': {
        if (type->isIntegerTy()) {
          return builder_.CreateAdd(L, R, "addtmp");
        } else if (type->isDoubleTy()) {
          return builder_.CreateFAdd(L, R, "addtmp");
        } else {
          break;
        }
      }
      
      case '-': {
        if (type->isIntegerTy()) {
          return builder_.CreateSub(L, R, "subtmp");
        } else if (type->isDoubleTy()) {
          return builder_.CreateFSub(L, R, "subtmp");
        } else {
          break;
        }
      }
      
      case '*': {
        if (type->isIntegerTy()) {
          return builder_.CreateMul(L, R, "multmp");
        } else if (type->isDoubleTy()) {
          return builder_.CreateFMul(L, R, "multmp");
        } else {
          break;
        }
      }
      
      case '<': {
        if (type->isIntegerTy()) {
          return builder_.CreateICmpULT(L, R, "cmptmp");
          //L = builder_.CreateFCmpULT(L, R, "cmptmp");
          // Convert bool 0/1 to double 0.0 or 1.0
          //return builder_.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "booltmp");
        } else if (type->isDoubleTy()) {
          return builder_.CreateFCmpULT(L, R, "cmptmp");
          //L = builder_.CreateFCmpULT(L, R, "cmptmp");
          // Convert bool 0/1 to double 0.0 or 1.0
          //return builder_.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "booltmp");
        } else {
          break;
        }
        
      }
      default: break;
    }
    
    return error("invalid binary operator");
  }
  
  
  // SymbolExpression
  Value *codegenSymbolExpression(SymbolExpression *symbolExpr) {
    DEBUG_TRACE_LLVM_VISITOR;
    assert(symbolExpr != 0);
    
    const Symbol& symbol = blockScope()->lookupSymbol(symbolExpr->name());
    
    if (symbol.empty()) return error("Unknown variable name");

    if (symbol.isAlloca()) {
      // Load the value.
      return builder_.CreateLoad(symbol.value, symbolExpr->name().c_str());
    } else {
      return symbol.value;
    }
  }


private:
  std::vector<std::string> errors_;
  std::vector<std::string> warnings_;
  Module* module_;
  IRBuilder<> builder_;
  BlockStack blockStack_;
};

}} // namespace rsms::codegen
#endif  // RSMS_CODEGEN_LLVM_VISITOR_H
