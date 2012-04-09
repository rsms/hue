#ifndef COVE_CODEGEN_H
#define COVE_CODEGEN_H

#include <stack>
#include <algorithm>
#include <map>

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Constants.h>
#include <llvm/GlobalVariable.h>
#include <llvm/Function.h>
#include <llvm/CallingConv.h>
#include <llvm/BasicBlock.h>
#include <llvm/Instructions.h>
#include <llvm/InlineAsm.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/GenericValue.h>

class NBlock;

class CodeGenBlock {
public:
    llvm::BasicBlock *block;
    std::map<std::string, llvm::Value*> locals;
};

class CodeGenContext {
    std::stack<CodeGenBlock *> blocks;
    llvm::Module *module_;

public:
    CodeGenContext() {
      module_ = new llvm::Module("main", llvm::getGlobalContext());
    }

    llvm::Module *module() const { return module_; };
    void generateCode(NBlock& root);
    llvm::GenericValue runCode();
    llvm::Type *toLLVMType(const NIdentifier& type);
    
    std::map<std::string, llvm::Value*>& locals() {
      return blocks.top()->locals;
    }
    
    llvm::BasicBlock *currentBlock() {
      return blocks.top()->block;
    }

    void pushBlock(llvm::BasicBlock *block) {
      blocks.push(new CodeGenBlock());
      blocks.top()->block = block;
    }
    
    void popBlock() {
      CodeGenBlock *top = blocks.top();
      blocks.pop();
      delete top;
    }
};

#endif // COVE_CODEGEN_H
