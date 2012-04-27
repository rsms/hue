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

#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include <vector>
#include <map>
#include <string>

namespace rsms { namespace codegen {
using namespace llvm;
using llvm::Function;
using llvm::FunctionType;
using llvm::Type;

/// CreateEntryBlockAlloca - Create an alloca instruction in the entry block of
/// the function.  This is used for variables etc.
static AllocaInst *CreateEntryBlockAlloca(BasicBlock *basicBlock, Type *type, const std::string &variableName) {
  IRBuilder<> TmpB(basicBlock, basicBlock->begin());
  // AllocaInst - an instruction to allocate memory on the stack
  return TmpB.CreateAlloca(type, 0, variableName.c_str());
}

class LLVMVisitor {
public:
  LLVMVisitor() : module_(NULL), builder_(getGlobalContext()) {}
  
  // Register an error
  Value *error(const char *str) {
    DEBUG_TRACE_LLVM_VISITOR;
    std::ostringstream ss;
    ss << str;
    errors_.push_back(ss.str());
    std::cerr << "\e[31;1m" << ss.str() << "\e[0m" << std::endl;
    return 0;
  }
  
  const std::vector<std::string>& errors() const { return errors_; };
  
  
  // Generate code for a module rooting at *root*
  llvm::Module *genModule(llvm::LLVMContext& context, std::string moduleName, const ast::Function *root) {
    DEBUG_TRACE_LLVM_VISITOR;
    llvm::Module *module = new Module(moduleName, context);
    
    module_ = module;
    //Value *moduleFunc = codegenFunction(root, std::string("minit__") + moduleName);
    Value *moduleFunc = codegenFunction(root, "main");
    module_ = 0;
    
    if (moduleFunc == 0) {
      delete module;
      module = 0;
    }
    
    return module;
  }
  
  
  // Emit LLVM IR for this AST node along with all the things it depends on.
  // "Value" is the class used to represent a
  // "Static Single Assignment (SSA) register" or "SSA value" in LLVM.
  Value *codegen(const ast::Node *node) {
    DEBUG_TRACE_LLVM_VISITOR;
    //std::cout << "Node [" << node->type << "]" << std::endl;
    switch (node->type) {
      //case Node::TBlock: return codegenBlock((ast::Block*)node);
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
                                           std::string name = "", Type *returnType = 0) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    // Find out the return type of the function
    if (returnType == 0) {
      TypeDeclarationList *returnTypes = node->returnTypes();
      if (returnTypes == 0 || returnTypes->size() == 0) {
        returnType = builder_.getVoidTy();
      } else {
        assert(returnTypes->size() == 1); // TODO: Support multiple return values
        ast::TypeDeclaration *astType = (*returnTypes)[0];
        switch (astType->type) {
          case ast::TypeDeclaration::Int:
            returnType = builder_.getInt64Ty();
            break;
          case ast::TypeDeclaration::Float:
            returnType = builder_.getDoubleTy();
            break;
          //case ast::TypeDeclaration::Func: return builder_.getDoubleTy();
          //case ast::TypeDeclaration::Named: return 1;
          default: {
            error("No encoding for AST type");
            return 0;
          }
        }
        
      }
    }
    
    // Make the function type:  double(double,double) etc.
    FunctionType *FT;
    VariableList *args = node->args();
    if (args != 0) {
      std::vector<llvm::Type*> floatArgs(args->size(), Type::getDoubleTy(getGlobalContext()));
      FT = FunctionType::get(returnType, floatArgs, /*isVararg = */false);
    } else {
      std::vector<llvm::Type*> noArgs();
      FT = FunctionType::get(returnType, /*isVararg = */false);
    }
  
    llvm::Function *F = Function::Create(FT, llvm::Function::ExternalLinkage, name, module_);
  
    // If F conflicted, there was already something named 'Name'.  If it has a
    // body, don't allow redefinition or reextern.
    if (F->getName() != name) {
      // Delete the one we just made and get the existing one.
      F->eraseFromParent();
      F = module_->getFunction(name);
    
      // If F already has a body, reject this.
      if (!F->empty()) {
        return (Function*)error("redefinition of function");
        return 0;
      }
    
      // If F took a different number of args, reject.
      if ( (args == 0 && F->arg_size() != 0) || (args && F->arg_size() != args->size()) ) {
        return (Function*)error("redefinition of function with different # args");
      }
    }
  
    // Set names for all arguments.
    if (args) {
      unsigned Idx = 0;
      for (llvm::Function::arg_iterator AI = F->arg_begin();
           Idx != args->size();
           ++AI, ++Idx) {
        const std::string& argName = (*args)[Idx]->name();
        AI->setName(argName);
    
        // Add arguments to variable symbol table.
        namedValues_[argName] = AI;
      }
    }
  
    return F;
  }
  
  
  // ExternalFunction
  Value *codegenExternalFunction(const ast::ExternalFunction* external) {
    DEBUG_TRACE_LLVM_VISITOR;
    return codegenFunctionInterface(external->interface(), external->name());
  }
  
  
  // Function
  Value *codegenFunction(const ast::Function *node, std::string name = "") {
    DEBUG_TRACE_LLVM_VISITOR;
    
    // Generate interface
    Type *returnType = builder_.getInt64Ty(); // todo
    llvm::Function *function = codegenFunctionInterface(node->interface(), name, returnType);
    if (function == 0) return 0;
  
    // Generate block code
    BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry", function);
    builder_.SetInsertPoint(BB);  // Set start insertion for subsequent builder inserts
    Value *lastValue = codegenBlock(node->body(), BB);
    if (lastValue == 0) {
      // Error reading body, remove function.
      function->eraseFromParent();
      return 0;
    }
    
    // Finish off the function by defining the return value
    // See CreateAggregateRet for returning multiple values
    // See CreateRetVoid for no return value
    builder_.CreateRet(lastValue);

    // Validate the generated code, checking for consistency.
    verifyFunction(*function);

    return function;
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
      if ((*it1)->type == Node::TAssignmentExpression && (
             ((AssignmentExpression*)(*it1))->rhs()->type == Node::TIntLiteralExpression
          || ((AssignmentExpression*)(*it1))->rhs()->type == Node::TFloatLiteralExpression
          || ((AssignmentExpression*)(*it1))->rhs()->type == Node::TBinaryExpression
          || ((AssignmentExpression*)(*it1))->rhs()->type == Node::TExternalFunction
        )) {
        lastValue = codegen(*it1);
        if (lastValue == 0) return 0;
      }
    }
    
    return lastValue;
  };
  
  
  // AssignmentExpression
  Value *codegenAssignment(const ast::AssignmentExpression* node) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    // Codegen the RHS.
    Value *rhsValue = codegen(node->rhs());
    if (rhsValue == 0) return 0;
    
    // Get the list of variables
    const VariableList *variables = node->variables();
    
    // For each variable
    assert(variables->size() == 1); // TODO: support >1 LHS variable
    VariableList::const_iterator it = variables->begin();
    for (; it < variables->end(); it++) {
      Variable *variable = (*it);
    
      // Look up the name.
      Value *variableValue = namedValues_[variable->name()];
      
      if (variableValue == 0) {
        // New variable
        Function *func = builder_.GetInsertBlock()->getParent();
      
        Type *allocaType = rhsValue->getType();
        AllocaInst *allocaInst = CreateEntryBlockAlloca(&func->getEntryBlock(), allocaType, variable->name());
        builder_.CreateStore(rhsValue, allocaInst);
        
        // Remember this binding.
        namedValues_[variable->name()] = allocaInst;
      } else if (variable->isMutable()) {
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
      Value *alias = namedValues_[node->calleeName()];
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
    if (calleeF->arg_size() != args.size())
      return error("Incorrect number of arguments passed");

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
    
    Value *L = codegen(binExpr->lhs());
    Value *R = codegen(binExpr->rhs());
    if (L == 0 || R == 0) return 0;
  
    switch (binExpr->operatorValue()) {
      case '+': return builder_.CreateFAdd(L, R, "addtmp");
      case '-': return builder_.CreateFSub(L, R, "subtmp");
      case '*': return builder_.CreateFMul(L, R, "multmp");
      case '<': {
        L = builder_.CreateFCmpULT(L, R, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        return builder_.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "booltmp");
      }
      default: return error("invalid binary operator");
    }
  }
  
  
  // SymbolExpression
  Value *codegenSymbolExpression(SymbolExpression *symbol) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    Value *V = namedValues_[symbol->name()];
    if (V == 0) return error("Unknown variable name");

    // Load the value.
    return builder_.CreateLoad(V, symbol->name().c_str());
  }


private:
  std::vector<std::string> errors_;
  Module *module_;
  IRBuilder<> builder_;
  std::map<std::string, llvm::Value*> namedValues_;
};

}} // namespace rsms::codegen
#endif  // RSMS_CODEGEN_LLVM_VISITOR_H
