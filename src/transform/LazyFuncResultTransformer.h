// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef _HUE_TRANSFORM_LFR_TRANSFORMER_INCLUDED
#define _HUE_TRANSFORM_LFR_TRANSFORMER_INCLUDED

#define DEBUG_TRANSFORM_LFR_VISITOR 1
#if DEBUG_LLVM_VISITOR
  #include "../DebugTrace.h"
  #define DEBUG_TRACE_LFR_VISITOR DEBUG_TRACE
#else
  #define DEBUG_TRACE_LFR_VISITOR do{}while(0)
#endif

#include <hue/Text.h>
#include <hue/ast/ast.h>
#include <string>
#include <map>
#include <deque>
#include <sstream>

#include "Scope.h"

namespace hue { namespace transform {

class LazyFuncResultTransformer : public Scoped {

  class Scope : public transform::Scope {
  public:
    Scope(Scoped* parent) : transform::Scope(parent), termType(0) {}
    ast::Type* termType;
  };


public:
  LazyFuncResultTransformer(ast::Block* block) : block_(block) {}

  Scope* currentScope() const { return (Scope*)Scoped::currentScope(); }

  bool run(std::string& errmsg) {
    assert(block_ != 0);
    bool ok = visitBlock(block_);
    if (!ok)
      errmsg.assign(errs_.str());
    errs_.str("");
    errs_.clear();
    return ok;
  }

  bool error(std::ostream& ss) {
    ss << std::endl;
    return false;
  }

  // ---------------------------------------------------------------

  bool visit(ast::Node* node) {
    DEBUG_TRACE_LFR_VISITOR;
    switch (node->nodeTypeID()) {
      #define VISIT(Name) \
        case ast::Node::T##Name: return visit##Name(static_cast<ast::Name*>(node));

      // Node types
      // VISIT(FunctionType);
      // VISIT(Variable);

      // Expression types
      VISIT(Function);
      //VISIT(ExternalFunction);
      VISIT(Block); // currently never hit
      VISIT(Symbol);
      VISIT(Assignment);
      VISIT(BinaryOp);
      VISIT(Call);
      VISIT(Conditional);
      //VISIT(IntLiteral);
      //VISIT(FloatLiteral);
      //VISIT(BoolLiteral);
      //VISIT(DataLiteral);
      //VISIT(TextLiteral);
      //VISIT(ListLiteral);

      #undef VISIT
      default:
        //rlog("Skipping node " << node->toString());
        return true;
    }
  }

  bool visitFunction(ast::Function *fun, const Text& name = "__func") {
    DEBUG_TRACE_LFR_VISITOR;

    //if (fun->functionType()->resultTypeIsUnknown()) rlog("Unresolved function " << name);

    Scope scope(this);

    // Define a reference to ourselves
    defineSymbol(name, fun);

    // Define argument symbols
    FunctionType *FT = fun->functionType();
    VariableList *args = FT->args();
    for (ast::VariableList::const_iterator I = args->begin(), E = args->end(); I != E; ++I) {
      defineSymbol((*I)->name(), *I);
    }

    return visitBlock(fun->body(), fun);
  }


  // bool visitExternalFunction(ast::ExternalFunction *fun, const Text& name = "") {
  //   DEBUG_TRACE_LFR_VISITOR;
  //   Scope scope(this);

  //   // Define a reference to ourselves
  //   defineSymbol(name, fun);

  //   return visitBlock(fun->body(), fun);
  // }


  bool visitBlock(ast::Block* block, ast::Function* parentFun = 0) {
    DEBUG_TRACE_LFR_VISITOR;
    Scope scope(this);

    ast::Expression* expression = 0;

    for (ExpressionList::const_iterator I = block->expressions().begin(),
                                        E = block->expressions().end();
         I != E; ++I)
    {
      expression = *I;
      if (!visit(expression)) return false;
    }

    //rlog("block->resultType(): " << block->resultType()->toString());
    return true;
  }


  bool visitConditional(ast::Conditional* cond) {
    DEBUG_TRACE_LFR_VISITOR;
    
    if (cond->testExpression() == 0) return error(errs_ << "Missing test expression in conditional");
    if (cond->trueBlock() == 0) return error(errs_ << "Missing true block in conditional");
    if (cond->falseBlock() == 0) return error(errs_ << "Missing false block in conditional");

    visit(cond->testExpression());
    visitBlock(cond->trueBlock());
    visitBlock(cond->falseBlock());

    // If we know the result type, set the result type of any unresolved dependants
    if (!cond->resultType()->isUnknown()) {
      cond->setResultType(cond->resultType());
    }

    return true;
  }


  bool visitCall(ast::Call* call) {
    DEBUG_TRACE_LFR_VISITOR;
    const Target& target = lookupSymbol(call->calleeName());

    if (target.isEmpty()) {
      return error(errs_ << "Unknown symbol \"" << call->calleeName() << "\"");
    }

    if (!target.value->isCallable()) {
      return error(errs_ << "Trying to call \"" << call->calleeName()
                   << "\" which is not a function");
    }
    
    // Substitute function for calleeName
    call->setCallee((ast::Function*)target.value);

    return true;
  }

  bool visitAssignment(ast::Assignment* node) {
    DEBUG_TRACE_LFR_VISITOR;
    const Text& name = node->variable()->name();
    Expression* rhs = node->rhs();
    
    defineSymbol(name, rhs);

    if (rhs->isFunction()) {
      ast::Function* F = static_cast<ast::Function*>(rhs);
      return visitFunction(F, name);
    //} else if (rhs->isExternalFunction()) {
    //  ast::ExternalFunction* F = static_cast<ast::ExternalFunction*>(rhs);
    //  return visitExternalFunction(F, name);
    } else {
      return visit(node->rhs());
    }
  }

  // Modifies symbol: setValue
  bool visitSymbol(ast::Symbol *sym) {
    DEBUG_TRACE_LFR_VISITOR;
    const Target& target = lookupSymbol(sym->name());
    if (target.isEmpty()) {
      return error(errs_ << "Unknown symbol \"" << sym->name() << "\"");
    } else {
      sym->setValue(target.value);
    }
    return true;
  }


  bool visitBinaryOp(ast::BinaryOp *binop) {
    DEBUG_TRACE_LFR_VISITOR;
    if (!visit(binop->lhs())) return false;
    return visit(binop->rhs());
  }


private:
  ast::Block* block_;
  std::ostringstream errs_;
  Scope* scope_;
};

}} // namespace hue transform
#endif // _HUE_TRANSFORM_LFR_TRANSFORMER_INCLUDED
