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
  
  Call(const Text &calleeName, ArgumentList &args)
    : Expression(TCall), calleeName_(calleeName), args_(args), callee_(0) {}

  const Text& calleeName() const { return calleeName_; }
  const ArgumentList& arguments() const { return args_; }

  Function* callee() const { return callee_; }
  void setCallee(Function* F) { callee_ = F; }

  virtual const Type *resultType() const {
    if (callee_ != 0) {
      return callee_->resultType() ? callee_->resultType() : &UnknownType;
    } else {
      return resultType_; // Type::Unknown
    }
  }

  virtual void setResultType(const Type* T) {
    assert(T->isUnknown() == false); // makes no sense to set T=unknown
    // Call setResultType with T for callee if callee's resultType is unknown
    if (callee_ && callee_->resultType()->isUnknown()) {
      callee_->setResultType(T);
    }
  }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "(";
    ss << calleeName_;
    for (ArgumentList::const_iterator I = args_.begin(), E = args_.end(); I != E; ++I) {
      ss << " " << (*I)->toString(level+1);
    }
    ss << ")";
    return ss.str();
  }
private:
  Text calleeName_;
  ArgumentList args_;
  Function* callee_; // weak
};

}} // namespace hue::ast
#endif // _HUE_AST_CALL_INCLUDED
