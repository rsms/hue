// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef HUE__CODEGEN_LLVM_VISITOR_H
#define HUE__CODEGEN_LLVM_VISITOR_H

#define DEBUG_LLVM_VISITOR 0
#if DEBUG_LLVM_VISITOR
  #include "../DebugTrace.h"
  #define DEBUG_TRACE_LLVM_VISITOR DEBUG_TRACE
#else
  #define DEBUG_TRACE_LLVM_VISITOR do{}while(0)
#endif

#include <hue/ast/ast.h>

#include "../Logger.h"
#include "../Text.h"

#include <stdlib.h>

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <tr1/unordered_map> // <map>
#include <deque>

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Support/IRBuilder.h>

namespace hue { namespace codegen {

class Visitor {
  class BlockScope;

  // Represents a named symbol (or "alias")
  class SymbolTarget {
  public:
    static SymbolTarget Empty;
    
    const ast::Type* hueType;
    llvm::Value *value;
    bool isMutable;
    BlockScope *owningScope;

    SymbolTarget() : hueType(0), value(0), isMutable(false), owningScope(0) {}
    //SymbolTarget(ast::Node* N, llvm::Value *V, bool M = true, BlockScope* S = 0)
    //  : astNode(N), value(V), isMutable(M), owningScope(S) {}
    
    inline bool isAlloca() const { return value ? llvm::AllocaInst::classof(value) : false; };
    inline bool empty() const { return value == 0; }
  };

  typedef std::map<Text, SymbolTarget> SymbolTargetMap;
  
  class FunctionSymbolTarget {
  public:
    ast::FunctionType* hueType;
    llvm::FunctionType* type;
    llvm::Value* value;
    BlockScope *owningScope;
  };
    
  typedef std::vector<FunctionSymbolTarget> FunctionSymbolTargetList;
  typedef std::map<Text, FunctionSymbolTargetList> FunctionSymbolTargetMap;
  
  // Iterable stack of block scopes
  typedef std::deque<BlockScope*> BlockStack;
  
  // Represents a scope of named symbols.
  class BlockScope {
  public:
    BlockScope(Visitor& visitor,
               llvm::BasicBlock *block,
               bool owningFHasLazyResult = false)
        : visitor_(visitor), block_(block) {
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
    
    inline llvm::BasicBlock *block() const { return block_; }

    inline const SymbolTargetMap& symbolTargets() const { return symbols_; }
    inline const FunctionSymbolTargetMap& functionsSymbolTargets() const { return functions_; }
    
    void setSymbolTarget(const Text& name, const ast::Type* type, llvm::Value *V, bool isMutable = true) {
      SymbolTarget& symbol = symbols_[name];
      symbol.hueType = type;
      symbol.value = V;
      symbol.isMutable = isMutable;
      symbol.owningScope = this;
    }
    
    bool setFunctionSymbolTarget(const Text& name, ast::FunctionType* hueT,
                                 llvm::FunctionType* T, llvm::Value *V);
    
    // Look up a symbol only in this scope.
    // Use Visitor::lookupSymbol to lookup stuff in any scope
    const SymbolTarget& lookupSymbolTarget(const Text& name) const {
      SymbolTargetMap::const_iterator it = symbols_.find(name);
      if (it != symbols_.end()) return it->second;
      return SymbolTarget::Empty;
    }
    
    // Look up a function symbols only in this scope.
    // Use Visitor::lookupFunctionSymbolTargets to lookup stuff in any scope
    const FunctionSymbolTargetList* lookupFunctionSymbolTargets(const Text& name) const {
      FunctionSymbolTargetMap::const_iterator it = functions_.find(name);
      if (it != functions_.end()) return &it->second;
      return 0;
    }
    
  private:
    Visitor& visitor_;
    llvm::BasicBlock *block_;
    SymbolTargetMap symbols_;
    FunctionSymbolTargetMap functions_;
  };
  
public:
  Visitor()
    : module_(NULL)
    , builder_(llvm::getGlobalContext())
    {}
  
  void setModule(llvm::Module* module) {
    module_ = module;
  }

  void reset() {
    module_ = 0;
    errors_.clear();
    warnings_.clear();
    builder_.ClearInsertionPoint();
    blockStack_.clear();
    arrayStructTypes_.clear();
  }

  // Register an error
  llvm::Value *error(const std::string& str) {
    std::ostringstream ss;
    ss << str;
    errors_.push_back(ss.str());
    std::cerr << "\e[31;1m" << ss.str() << "\e[0m" << std::endl;
    return 0;
  }
  
  // Register a warning
  llvm::Value *warning(const std::string& str) {
    std::ostringstream ss;
    ss << str;
    warnings_.push_back(ss.str());
    std::cerr << "\e[35;1mWarning:\e[0m " << ss.str() << std::endl;
    return 0;
  }
  
  inline const std::vector<std::string>& errors() const { return errors_; };
  inline const std::vector<std::string>& warnings() const { return warnings_; };
  
  // Generate code for a module rooting at *root*
  llvm::Module *genModule(llvm::LLVMContext& context, const Text moduleName, ast::Function *root);
  
  static llvm::FunctionType* functionTypeForValue(llvm::Value* V);
  
protected:
  // Current block scope, or 0 if none
  inline BlockScope* blockScope() const { return blockStack_.empty() ? 0 : blockStack_.back(); }
  
  // Current block, or 0 if none
  inline llvm::BasicBlock* block() const { return builder_.GetInsertBlock(); }
  
  const SymbolTarget& lookupSymbol(const Text& name) const {
    // Scan symbol maps starting at top of stack moving down
    BlockStack::const_reverse_iterator bsit = blockStack_.rbegin();
    for (; bsit != blockStack_.rend(); ++bsit) {
      BlockScope* bs = *bsit;
      SymbolTargetMap::const_iterator it = bs->symbolTargets().find(name);
      if (it != bs->symbolTargets().end()) {
        return it->second;
      }
    }
    return SymbolTarget::Empty;
  }
  
  FunctionSymbolTargetList lookupFunctionSymbols(const ast::Symbol& symbol) const;

  bool moduleHasNamedGlobal(llvm::StringRef name) const {
    return module_->getNamedValue(name) != 0
        || module_->getNamedGlobal(name) != 0;
  }

  typedef enum {
    CandidateErrorArgCount = 0,
    CandidateErrorArgTypes,
    CandidateErrorReturnType,
    CandidateErrorAmbiguous,
  } CandidateError;

  std::string formatFunctionCandidateErrorMessage(const ast::Call* node,
                                                  const FunctionSymbolTargetList& candidates,
                                                  CandidateError error) const;
  
  //std::vector<Function> lookupFunctions(const Text& name);
  
  // Dump all symbols in the current block stack to stderr
  void dumpBlockSymbols();
  
  
  //llvm::Type* returnTypeForFunctionType(const ast::FunctionType *node) {
  //  // Find out the return type of the function
  //  ast::Type *returnType = node->resultType();
  //  return (returnType == 0) ? builder_.getVoidTy() : IRTypeForASTType(returnType);
  //}

  // The following are implemented in type_conversion.cc
  llvm::StructType* getArrayStructType(llvm::Type* elementType);
  inline llvm::StructType* getI8ArrayStructType() { return getArrayStructType(builder_.getInt8Ty()); }
  llvm::StructType* getExplicitStructType(const ast::StructType* astST);
  llvm::FunctionType* getLLVMFuncTypeForASTFuncType(const ast::FunctionType* astFT,
                                                    llvm::Type* returnType = 0);
  
  // The following are implemented in type_conversion.cc
  llvm::Type *IRTypeForASTType(const ast::Type* T);
  llvm::ArrayType* getArrayTypeFromPointerType(llvm::Type* T) const;
  ast::Type::TypeID ASTTypeIDForIRType(const llvm::Type* T);
  ast::Type *ASTTypeForIRType(const llvm::StructType* T);
  ast::Type *ASTTypeForIRType(const llvm::Type* T);
  // Returns false if an error occured
  bool IRTypesForASTVariables(std::vector<llvm::Type*>& argSpec, ast::VariableList *argVars);
  
  llvm::Type* returnTypeForFunctionType(const ast::FunctionType* astFT);

  llvm::Value* wrapValueInGEP(llvm::Value* V) {
    llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0);
    llvm::Value *Args[] = { zero, zero };
    //return builder_.CreateInBoundsGEP(gV, Args, "ptrtmp");
    return builder_.CreateGEP(V, Args, "ptrtmp");
  }
  
  const char* typeName(const llvm::Type* T) const;
  
  
  llvm::Value* createNewLocalSymbol(const ast::Variable *variable,
                                    const ast::Type* hueType,
                                    llvm::Value *rhsV,
                                    bool warnRedundantTypeDecl = true);
  
  inline std::string mangledName(const std::string& localName) const {
    return module_->getModuleIdentifier() + ":" + localName;
  }
  
  // Returns a module-global uniqe mangled name rooted in *name*
  std::string uniqueMangledName(const Text& name) const;
  
  // Create an alloca of type T
  llvm::AllocaInst *createAlloca(llvm::Type* T, const Text& name) {
    llvm::IRBuilder<> TmpB(block(), block()->begin());
    return TmpB.CreateAlloca(T, 0, name.UTF8String());
  }
  
  // Create an alloca of type V->getType and store V into that alloca
  llvm::AllocaInst *createAllocaAndStoreValue(llvm::Value* V, const Text& name, llvm::StoreInst** storeInst = 0) {
    llvm::AllocaInst *allocaInst = createAlloca(V->getType(), name);
    if (allocaInst) {
      llvm::StoreInst* si = builder_.CreateStore(V, allocaInst);
      if (storeInst != 0) *storeInst = si;
    }
    return allocaInst;
  }
  
  llvm::Type* highestFidelityType(llvm::Type* T1, llvm::Type* T2);
  llvm::Value* castValueToBool(llvm::Value* V);
  llvm::Value* castValueTo(llvm::Value* V, llvm::Type* destT);
  
  llvm::GlobalVariable* createPrivateConstantGlobal(llvm::Constant* constantV, const llvm::Twine &name = "");
  llvm::GlobalVariable* createStruct(llvm::Constant** constants, size_t count, const llvm::Twine &name = "");
  llvm::GlobalVariable* createArray(llvm::Constant* constantArray, const llvm::Twine &name = "");
  
  // ------------------------------------------------

public:
  
  // Emit LLVM IR for this AST node along with all the things it depends on.
  // "Value" is the class used to represent a
  // "Static Single Assignment (SSA) register" or "SSA value" in LLVM.
  llvm::Value *codegen(ast::Node *node) {
    DEBUG_TRACE_LLVM_VISITOR;
    //std::cout << "Node [" << node->nodeTypeID() << "]" << std::endl;
    switch (node->nodeTypeID()) {
      #define HANDLE(Name) case ast::Node::T##Name: return codegen##Name(static_cast<ast::Name*>(node));
      HANDLE(Symbol);
      HANDLE(BinaryOp);
      HANDLE(Function);
      HANDLE(IntLiteral);
      HANDLE(FloatLiteral);
      HANDLE(BoolLiteral);
      HANDLE(DataLiteral);
      HANDLE(TextLiteral);
      HANDLE(Assignment);
      HANDLE(Call);
      HANDLE(Conditional);
      HANDLE(Structure);
      #undef HANDLE
      default: return error(std::string("Unable to generate code for node ")+node->toString());
    }
  }
  
  llvm::Function *codegenFunctionType(ast::FunctionType *node,
                                      std::string name = "",
                                      llvm::Type *returnType = 0);
  
  llvm::Value *codegenExternalFunction(const ast::ExternalFunction* node);
  
  llvm::Value *codegenFunction(ast::Function *node,
                               const Text& symbol = "",
                               std::string name = "",
                               llvm::Type* returnType = 0,
                               llvm::Value* returnValue = 0);
  
  llvm::Value *codegenBlock(const ast::Block *block);
  llvm::Value *codegenAssignment(const ast::Assignment* node);
  llvm::Value *codegenCall(const ast::Call* node, const ast::Type* expectedReturnType = 0);
  llvm::Value *codegenConditional(const ast::Conditional* node);
  llvm::Value *codegenIntLiteral(const ast::IntLiteral *literal, bool fixedSize = true);
  llvm::Value *codegenFloatLiteral(const ast::FloatLiteral *literal, bool fixedSize = true);
  llvm::Value *codegenBoolLiteral(const ast::BoolLiteral *literal);
  llvm::Value *codegenDataLiteral(const ast::DataLiteral *literal);
  llvm::Value *codegenTextLiteral(const ast::TextLiteral *literal);
  llvm::Value *codegenBinaryOp(const ast::BinaryOp *binExpr);
  llvm::Value *codegenSymbol(const ast::Symbol *symbolExpr);

  llvm::Value *codegenStructure(const ast::Structure *expr, const Text& name = "");
  llvm::Value* codegenConstantStructure(llvm::StructType *ST,
    const std::vector<llvm::Constant*>& fields, const Text& name);
  llvm::Value* codegenDynamicStructure(llvm::StructType *ST,
    const std::vector<llvm::Value*>& fields, const Text& name);


private:
  std::vector<std::string> errors_;
  std::vector<std::string> warnings_;
  llvm::Module* module_;
  llvm::IRBuilder<> builder_;
  BlockStack blockStack_;
  std::map<llvm::Type*, llvm::StructType*> arrayStructTypes_;
};

}} // namespace hue::codegen
#endif  // HUE__CODEGEN_LLVM_VISITOR_H
