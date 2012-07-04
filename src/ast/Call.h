// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#ifndef _HUE_AST_CALL_INCLUDED
#define _HUE_AST_CALL_INCLUDED

#include "Expression.h"
#include "Function.h"

namespace hue { namespace ast {

// Function calls.
class Call : public Expression {
public:
  typedef std::vector<Expression*> ArgumentList;
  
  Call(Symbol* calleeSymbol, ArgumentList &args)
    : Expression(TCall), calleeSymbol_(calleeSymbol), args_(args), calleeType_(0) {}

  Symbol* symbol() const { return calleeSymbol_; }
  const ArgumentList& arguments() const { return args_; }

  const FunctionType* calleeType() const { return calleeType_; }
  void setCalleeType(const FunctionType* FT) { calleeType_ = FT; }

  virtual const Type *resultType() const {
    return calleeType_ ? calleeType_->resultType() : &UnknownType;
  }

  virtual void setResultType(const Type* T) {
    assert(T->isUnknown() == false); // makes no sense to set T=unknown
    // Call setResultType with T for callee if callee's resultType is unknown
    if (calleeType_ && calleeType_->resultType()->isUnknown()) {
      const_cast<FunctionType*>(calleeType_)->setResultType(T);
    }
  }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    //NodeToStringHeader(level, ss);
    ss << "(";
    ss << calleeSymbol_;
    for (ArgumentList::const_iterator I = args_.begin(), E = args_.end(); I != E; ++I) {
      ss << " " << (*I)->toString(level+1);
    }
    ss << ")";
    return ss.str();
  }
private:
  Symbol* calleeSymbol_;
  ArgumentList args_;
  const FunctionType* calleeType_; // weak
};

}} // namespace hue::ast
#endif // _HUE_AST_CALL_INCLUDED
