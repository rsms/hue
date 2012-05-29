#include "Symbol.h"
#include "Function.h"
#include "../Logger.h"

namespace hue { namespace ast {

Type *Symbol::resultType() const {
  if (value_ && value_->isExpression()) {
    return static_cast<Expression*>(value_)->resultType();
  } else if (value_ && value_->isFunctionType()) {
    return static_cast<FunctionType*>(value_)->resultType();
  } else if (value_ && value_->isVariable()) {
    Variable* var = static_cast<Variable*>(value_);
    return var->type() ? var->type() : resultType_;
  } else {
    return resultType_; // Type::Unknown;
  }
}

void Symbol::setResultType(Type* T) {
  assert(T->isUnknown() == false); // makes no sense to set T=unknown
  // Call setResultType with T for value_ if value_ result type is unknown
  if (value_) {

    if (value_->isExpression()) {
      Expression* expr = static_cast<Expression*>(value_);
      if (expr->resultType()->isUnknown())
        expr->setResultType(T);

    } else if (value_->isFunctionType()) {
      FunctionType* FT = static_cast<FunctionType*>(value_);
      if (FT->resultType()->isUnknown())
        FT->setResultType(T);

    } else if (value_->isVariable()) {
      Variable* var = static_cast<Variable*>(value_);
      if (!var->type() || var->type()->isUnknown())
        var->setType(T);
    }

  }
}

}} // namespace hue::ast
