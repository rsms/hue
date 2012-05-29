// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#ifndef HUE__AST_VARIABLE_DEFINITION_H
#define HUE__AST_VARIABLE_DEFINITION_H
#include "Node.h"
#include "Type.h"
#include <vector>
#include <string>
namespace hue { namespace ast {

class Variable;
typedef std::vector<Variable*> VariableList;

class Variable : public Node {
public:
  Variable(bool isMutable, const Text& name, Type *type)
    : Node(TVariable), isMutable_(isMutable), name_(name), type_(type) {}
  
  // Primarily used for tests:
  Variable(Type *type)
    : Node(TVariable), isMutable_(false), name_(), type_(type) {}
  
  const bool& isMutable() const { return isMutable_; }
  const Text& name() const { return name_; }
  
  const Type *type() const { return type_; }
  bool hasUnknownType() const { return !type_ || type_->typeID() == Type::Unknown; }
  void setType(const Type *type) { type_ = type; }
  
  std::string toString(int level = 0) const {
    std::ostringstream ss;
    //NodeToStringHeader(level, ss);
    ss << "<Variable "
       << name_ << ' '
       << (isMutable_ ? "MUTABLE " : "")
       << (type_ ? type_->toString() : "?")
       << '>';
    return ss.str();
  }
  
private:
  bool isMutable_;
  Text name_;
  const Type *type_;
};




}} // namespace hue::ast
#endif  // HUE__AST_VARIABLE_DEFINITION_H
