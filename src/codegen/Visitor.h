#ifndef HUE__CODEGEN_LLVM_VISITOR_H
#define HUE__CODEGEN_LLVM_VISITOR_H

#define DEBUG_LLVM_VISITOR 1
#if DEBUG_LLVM_VISITOR
  #include "../DebugTrace.h"
  #define DEBUG_TRACE_LLVM_VISITOR DEBUG_TRACE
#else
  #define DEBUG_TRACE_LLVM_VISITOR do{}while(0)
#endif

#include "../ast/Node.h"
#include "../ast/Expression.h"
#include "../ast/Function.h"
#include "../ast/Block.h"
#include "../ast/Type.h"
#include "../ast/Variable.h"
#include "../ast/Conditional.h"
#include "../ast/DataLiteral.h"
#include "../ast/TextLiteral.h"

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
  class Symbol {
  public:
    static Symbol Empty;
    llvm::Value *value;
    bool isMutable;
    BlockScope *owningScope;
    inline bool isAlloca() const { return value ? llvm::AllocaInst::classof(value) : false; };
    inline bool empty() const { return value == 0; }
    Symbol() : value(0), isMutable(true), owningScope(0) {}
    Symbol(llvm::Value *V, bool M = true, BlockScope* S = 0) : value(V), isMutable(M), owningScope() {}
  };
  
  // Iterable stack of block scopes
  typedef std::deque<BlockScope*> BlockStack;
  
  // Represents a scope of named symbols.
  class BlockScope {
  public:
    typedef std::map<Text,Symbol> SymbolMap;
    //typedef std::tr1::unordered_map<Text,Symbol> SymbolMap;
    
    BlockScope(Visitor& visitor, llvm::BasicBlock *block) : visitor_(visitor), block_(block) {
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
    inline const SymbolMap& symbols() const { return symbols_; }
    
    void setSymbol(const Text& name, llvm::Value *V, bool isMutable = true) {
      Symbol& symbol = symbols_[name];
      symbol.value = V;
      symbol.isMutable = isMutable;
      symbol.owningScope = this;
    }
    
    // Look up a symbol only in this scope.
    // Use Visitor::lookupSymbol to lookup stuff in any scope
    const Symbol& lookupSymbol(const Text& name, bool deep = true) const {
      SymbolMap::const_iterator it = symbols_.find(name);
      if (it != symbols_.end()) return it->second;
      return Symbol::Empty;
    }
    
  private:
    Visitor& visitor_;
    llvm::BasicBlock *block_;
    SymbolMap symbols_;
  };
  
public:
  Visitor() : module_(NULL), builder_(llvm::getGlobalContext()) {}
  
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
  llvm::Module *genModule(llvm::LLVMContext& context, const Text moduleName, const ast::Function *root);
  
  
protected:
  // Current block scope, or 0 if none
  inline BlockScope* blockScope() const { return blockStack_.empty() ? 0 : blockStack_.back(); }
  
  // Current block, or 0 if none
  inline llvm::BasicBlock* block() const { return builder_.GetInsertBlock(); }
  
  const Symbol& lookupSymbol(const Text& name, bool deep = true) const {
    // Scan symbol maps starting at top of stack moving down
    BlockStack::const_reverse_iterator bsit = blockStack_.rbegin();
    for (; bsit != blockStack_.rend(); ++bsit) {
      BlockScope* bs = *bsit;
      BlockScope::SymbolMap::const_iterator it = bs->symbols().find(name);
      if (it != bs->symbols().end()) {
        return it->second;
      }
    }
    return Symbol::Empty;
  }
  
  llvm::Value *resolveSymbol(const Text& name);
  
  // Dump all symbols in the current block stack to stderr
  void dumpBlockSymbols();
  
  
  llvm::Type* returnTypeForFunctionType(const ast::FunctionType *node) {
    // Find out the return type of the function
    ast::TypeList *returnTypes = node->returnTypes();
    if (returnTypes == 0 || returnTypes->size() == 0 /*TODO: || size==1 && type==TNull */) {
      return builder_.getVoidTy();
    } else {
      assert(returnTypes->size() == 1); // TODO: Support multiple return values
      ast::Type *parsedT = (*returnTypes)[0];
      return IRTypeForASTType(parsedT);
    }
  }
  
  llvm::Type *IRTypeForASTType(const ast::Type* T) {
    switch (T->typeID()) {
      case ast::Type::Float:  return builder_.getDoubleTy();
      case ast::Type::Int:    return builder_.getInt64Ty();
      case ast::Type::Char:   return builder_.getInt32Ty();
      case ast::Type::Byte:   return builder_.getInt8Ty();
      case ast::Type::Bool:   return builder_.getInt1Ty();
      case ast::Type::Array: {
        llvm::Type* elementType = IRTypeForASTType((reinterpret_cast<const ast::ArrayType*>(T))->type());
        //return llvm::PointerType::get(elementType, 0);
        // Return a pointer to a struct of the array type: <{ i64, elementType* }>*
        return llvm::PointerType::get(getArrayStructType(elementType), 0);
      }
      //case ast::Type::Func: ...
      //case ast::Type::Named: ...
      default: return 0;
    }
  }
  
  llvm::ArrayType* getArrayTypeFromPointerType(llvm::Type* T) const {
    return (    T->isPointerTy()
             && T->getNumContainedTypes() == 1
             && llvm::ArrayType::classof(T = T->getContainedType(0))
              ? static_cast<llvm::ArrayType*>(T) : 0 );
  }
  
  llvm::Value* wrapValueInGEP(llvm::Value* V) {
    llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm::getGlobalContext()), 0);
    llvm::Value *Args[] = { zero, zero };
    //return builder_.CreateInBoundsGEP(gV, Args, "ptrtmp");
    return builder_.CreateGEP(V, Args, "ptrtmp");
  }
  
  const char* typeName(const llvm::Type* T) const;
  
  // Returns false if an error occured
  bool IRTypesForASTVariables(std::vector<llvm::Type*>& argSpec, ast::VariableList *argVars);
  
  llvm::Value* createNewLocalSymbol(ast::Variable *variable, llvm::Value *rhsV);
  
  inline std::string mangledName(const Text& localName) const {
    return module_->getModuleIdentifier() + "$" + localName.UTF8String();
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
  
  llvm::StructType* getArrayStructType(llvm::Type* elementType);
  inline llvm::StructType* getI8ArrayStructType() { return getArrayStructType(builder_.getInt8Ty()); }
  
  llvm::GlobalVariable* createPrivateConstantGlobal(llvm::Constant* constantV, const llvm::Twine &name = "");
  llvm::GlobalVariable* createStruct(llvm::Constant** constants, size_t count, const llvm::Twine &name = "");
  llvm::GlobalVariable* createArray(llvm::Constant* constantArray, const llvm::Twine &name = "");
  
  // ------------------------------------------------
  
  // Emit LLVM IR for this AST node along with all the things it depends on.
  // "Value" is the class used to represent a
  // "Static Single Assignment (SSA) register" or "SSA value" in LLVM.
  llvm::Value *codegen(const ast::Node *node) {
    DEBUG_TRACE_LLVM_VISITOR;
    //std::cout << "Node [" << node->nodeTypeID() << "]" << std::endl;
    switch (node->nodeTypeID()) {
      #define HANDLE(Name) case ast::Node::T##Name: return codegen##Name(static_cast<const ast::Name*>(node));
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
      #undef HANDLE
      default: return error(std::string("Unable to generate code for node ")+node->toString());
    }
  }
  
  llvm::Function *codegenFunctionType(const ast::FunctionType *node,
                                      std::string name = "",
                                      llvm::Type *returnType = 0);
  
  llvm::Value *codegenExternalFunction(const ast::ExternalFunction* node);
  
  llvm::Value *codegenFunction(const ast::Function *node,
                               std::string name = "",
                               llvm::Type* returnType = 0,
                               llvm::Value* returnValue = 0);
  
  llvm::Value *codegenBlock(const ast::Block *block);
  
  llvm::Value *codegenAssignment(const ast::Assignment* node);
  
  llvm::Value *codegenCall(const ast::Call* node);
  llvm::Value *codegenConditional(const ast::Conditional* node);
  
  llvm::Value *codegenIntLiteral(const ast::IntLiteral *literal, bool fixedSize = true);
  llvm::Value *codegenFloatLiteral(const ast::FloatLiteral *literal, bool fixedSize = true);
  llvm::Value *codegenBoolLiteral(const ast::BoolLiteral *literal);
  llvm::Value *codegenDataLiteral(const ast::DataLiteral *literal);
  llvm::Value *codegenTextLiteral(const ast::TextLiteral *literal);
  
  llvm::Value *codegenBinaryOp(const ast::BinaryOp *binExpr);
  
  llvm::Value *codegenSymbol(const ast::Symbol *symbolExpr);


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
