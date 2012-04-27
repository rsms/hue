#ifndef RSMS_CODEGEN_LLVM_VISITOR_H
#define RSMS_CODEGEN_LLVM_VISITOR_H

#include "../DebugTrace.h"
#define DEBUG_TRACE_LLVM_VISITOR DEBUG_TRACE
//#define DEBUG_TRACE_LLVM_VISITOR do{}while(0)

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
      //case Node::TFunctionInterface: return codegenFunctionInterface((ast::FunctionInterface*)node);
      case Node::TFunction: return codegenFunction((ast::Function*)node);
      case Node::TIntLiteralExpression: return codegenIntLiteral((IntLiteralExpression*)node);
      case Node::TAssignmentExpression: return codegenAssignment((AssignmentExpression*)node);
      default: return error("Unable to generate code for node");
    }
  }
  
  
  // FunctionInterface
  llvm::Function *codegenFunctionInterface(const ast::FunctionInterface *node,
                                           Type *returnType,
                                           std::string name = "") {
    DEBUG_TRACE_LLVM_VISITOR;
    
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
  
  
  // Function
  Value *codegenFunction(const ast::Function *node, std::string name = "") {
    DEBUG_TRACE_LLVM_VISITOR;
    
    // Generate interface
    Type *returnType = builder_.getInt64Ty(); // todo
    llvm::Function *function = codegenFunctionInterface(node->interface(), returnType, name);
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
      if ((*it1)->type == Node::TAssignmentExpression
          && ((AssignmentExpression*)(*it1))->rhs()->type == Node::TIntLiteralExpression) {
        lastValue = codegen(*it1);
        if (lastValue == 0) return 0;
      }
    }
    
    return lastValue;
  };
  
  
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
        return error("not implemented (value mutation)");
      } else {
        return error("Trying to mutate an immutable value");
      }
    
    }

    return rhsValue;
  }
  
  
  Value *codegenIntLiteral(const ast::IntLiteralExpression *intLiteral, bool isMutable = true) {
    DEBUG_TRACE_LLVM_VISITOR;
    
    const unsigned numBits = 64; // TODO: Infer the minimal size needed if isMutable is false
    return ConstantInt::get(getGlobalContext(), APInt(numBits, intLiteral->text(), intLiteral->radix()));
  }

private:
  std::vector<std::string> errors_;
  Module *module_;
  IRBuilder<> builder_;
  std::map<std::string, llvm::Value*> namedValues_;
};

}} // namespace rsms::codegen
#endif  // RSMS_CODEGEN_LLVM_VISITOR_H
