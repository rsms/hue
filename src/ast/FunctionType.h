// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef HUE_AST_FUNCTION_TYPE_H
#define HUE_AST_FUNCTION_TYPE_H

#include "Type.h"
#include "Variable.h"
#include <string>
#include <vector>
#include <map>

namespace hue { namespace ast {

// Represents the "prototype" for a function, which captures its name, and its
// argument names (thus implicitly the number of arguments the function takes).
class FunctionType : public Type {
public:
  FunctionType(VariableList *args = 0,
               const Type *resultType = 0,
               bool isPublic = false)
    : Type(FuncT), args_(args), resultType_(resultType), isPublic_(isPublic) {}

  VariableList *args() const { return args_; }

  const Type *resultType() const { return resultType_; }
  void setResultType(const Type* T) { resultType_ = T; }

  bool resultTypeIsUnknown() const { return resultType_ && resultType_->isUnknown(); }
  
  bool isPublic() const { return isPublic_; }
  void setIsPublic(bool isPublic) { isPublic_ = isPublic; }
  
  std::string toString(int level = 0) const;
  virtual std::string toString() const { return toString(0); }
  virtual std::string toHueSource() const;
  
private:
  VariableList *args_;
  const Type *resultType_;
  bool isPublic_;
};

}} // namespace hue::ast
#endif  // HUE_AST_FUNCTION_TYPE_H
