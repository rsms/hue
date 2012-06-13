#include "Symbol.h"
#include "Function.h"
#include "../Logger.h"
#include <assert.h>

namespace hue { namespace ast {


Symbol::Symbol(const Text &name, bool isPath) : Expression(TSymbol) {
  if (!isPath) {
    pathname_.push_back(name);
  } else {
    pathname_ = name.split(':');
  }
}


std::string Symbol::toString(int level) const {
  if (pathname_.size() == 0) {
    return "";
  } else if (pathname_.size() == 1) {
    return pathname_[0].UTF8String();
  } else {
    return Text(":").join(pathname_).UTF8String();
  }
}


// From when we kept a ref to the value:
// const Type *Symbol::resultType() const {
//   if (value_ && value_->isExpression()) {
//     return static_cast<Expression*>(value_)->resultType();
//   } else if (value_ && value_->isFunctionType()) {
//     return static_cast<FunctionType*>(value_)->resultType();
//   } else if (value_ && value_->isVariable()) {
//     Variable* var = static_cast<Variable*>(value_);
//     return var->type() ? var->type() : resultType_;
//   } else {
//     return resultType_;
//   }
// }

// void Symbol::setResultType(const Type* T) {
//   assert(T->isUnknown() == false); // makes no sense to set T=unknown

//   // Call setResultType with T for value_ if value_ result type is unknown
//   if (value_) {

//     if (value_->isExpression()) {
//       Expression* expr = static_cast<Expression*>(value_);
//       if (expr->resultType()->isUnknown())
//         expr->setResultType(T);

//     } else if (value_->isFunctionType()) {
//       FunctionType* FT = static_cast<FunctionType*>(value_);
//       if (FT->resultType()->isUnknown())
//         FT->setResultType(T);

//     } else if (value_->isVariable()) {
//       Variable* var = static_cast<Variable*>(value_);
//       if (!var->type() || var->type()->isUnknown())
//         var->setType(T);
//     }

//   } else {
//     resultType_ = T;
//   }
// }

}} // namespace hue::ast
