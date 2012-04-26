#ifndef RSMS_CODEGEN_LLVM_VISITOR_H
#define RSMS_CODEGEN_LLVM_VISITOR_H

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

class LLVMVisitor {
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
  
  
  // Generate code for a module rooting at *root*
  llvm::Module *genModule(llvm::LLVMContext& context, std::string moduleName, ast::Function *root) {
    llvm::Module *module = new Module(moduleName, context);
    
    module_ = module;
    codegen(root);
    module_ = NULL;
    
    return module;
  }
  
  
  // Emit LLVM IR for this AST node along with all the things it depends on.
  // "Value" is the class used to represent a
  // "Static Single Assignment (SSA) register" or "SSA value" in LLVM.
  Value *codegen(ast::Node *node) {
    std::cout << "Node" << std::endl;
    error("Unknown node type");
    return 0;
  }
  
  
  // FunctionInterface
  llvm::Function *codegen(ast::FunctionInterface *node) {
    using llvm::Function;
    std::cout << "FunctionInterface" << std::endl;
    
    // Make the function type:  double(double,double) etc.
    VariableList *args = node->args();
    std::vector<Type*> Doubles(args->size(), Type::getDoubleTy(getGlobalContext()));
    FunctionType *FT = FunctionType::get(Type::getDoubleTy(getGlobalContext()), Doubles, false);
  
    std::string functionName("foobar");
    llvm::Function *F = Function::Create(FT, llvm::Function::ExternalLinkage, functionName, module_);
  
    // If F conflicted, there was already something named 'Name'.  If it has a
    // body, don't allow redefinition or reextern.
    if (F->getName() != functionName) {
      // Delete the one we just made and get the existing one.
      F->eraseFromParent();
      F = module_->getFunction(functionName);
    
      // If F already has a body, reject this.
      if (!F->empty()) {
        return (Function*)error("redefinition of function");
        return 0;
      }
    
      // If F took a different number of args, reject.
      if (F->arg_size() != args->size()) {
        return (Function*)error("redefinition of function with different # args");
      }
    }
  
    // Set names for all arguments.
    unsigned Idx = 0;
    for (llvm::Function::arg_iterator AI = F->arg_begin();
         Idx != args->size();
         ++AI, ++Idx) {
      const std::string& argName = (*args)[Idx]->name();
      AI->setName(argName);
    
      // Add arguments to variable symbol table.
      namedValues_[argName] = AI;
    }
  
    return F;
  }
  
  
  // Function
  Value *codegen(ast::Function *node) {
    std::cout << "Function" << std::endl;
    
    // Generate interface
    llvm::Function *function = codegen(node->interface());
    if (function == 0) return 0;
    
    // Create a new basic block to start insertion into.
    BasicBlock *bb = BasicBlock::Create(getGlobalContext(), "entry", function);
    builder_.SetInsertPoint(bb);
  
    // Generate block code
    Value *bodyValue = codegen(node->body());
    if (bodyValue == 0) {
      // Error reading body, remove function.
      function->eraseFromParent();
      return 0;
    }

    // Finish off the function.
    builder_.CreateRet(bodyValue);

    // Validate the generated code, checking for consistency.
    verifyFunction(*function);

    return function;
  }
  
  
  // Block
  Value *codegen(ast::Block *node) {
    std::cout << "Block" << std::endl;
    return 0;
  };
  
  //return ConstantFP::get(getGlobalContext(), APFloat(Val));

private:
  std::vector<std::string> errors_;
  Module *module_;
  IRBuilder<> builder_;
  std::map<std::string, llvm::Value*> namedValues_;
};

}} // namespace rsms::codegen
#endif  // RSMS_CODEGEN_LLVM_VISITOR_H
