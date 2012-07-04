// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#ifndef HUE__AST_PROTOTYPE_H
#define HUE__AST_PROTOTYPE_H
#include "Node.h"
#include "Expression.h"
#include "Type.h"
#include "FunctionType.h"
#include "Block.h"

namespace hue { namespace ast {

class Argument {
public:
  Text identifier;
  Type type;
  
  Argument(Text identifier, const Type& type)
    : identifier(identifier), type(type) {}

  Argument(Text identifier) : identifier(identifier), type(Type::Unknown) {}
  
  std::string toString() const {
    return identifier.UTF8String() + " " + type.toString();
  }
};

// Represents a function definition.
class Function : public Expression {
  FunctionType *functionType_;
  Block *body_;
public:
  Function(FunctionType *functionType, Block *body)
    : Expression(TFunction), functionType_(functionType), body_(body) {}
  Function(FunctionType *functionType, Expression *body)
    : Expression(TFunction), functionType_(functionType), body_(new Block(body)) {}

  FunctionType *functionType() const { return functionType_; }
  Block *body() const { return body_; }

  // Override Expression result type
  virtual const Type *resultType() const {
    return functionType_;
  }
  virtual void setResultType(const Type* T) throw(std::logic_error) {
    throw std::logic_error("Can not set type for compound 'Function' expression");
  }

  // Type resulting from a call to this function
  const Type *callResultType() const {
    return functionType_ ? functionType_->resultType() : &UnknownType;
  }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    //NodeToStringHeader(level, ss);
    ss << '(' << (functionType_ ? functionType_->toString(level) : "<no function type>")
       << " ";
    NodeToStringHeader(level, ss);
    ss << (body_ ? body_->toString(level) : "nil")
       << ')';
    return ss.str();
  }
};

// Represents an external function declaration.
// TODO: Join Function and ExternalFunction classes
class ExternalFunction : public Expression {
  Text name_;
  FunctionType *functionType_;
public:
  ExternalFunction(const Text &name, FunctionType *functionType)
    : Expression(TExternalFunction), name_(name), functionType_(functionType) {}
  
  inline const Text& name() const { return name_; }
  inline FunctionType *functionType() const { return functionType_; }

  // Override Expression result type
  virtual const Type *resultType() const {
    return functionType_ ? functionType_->resultType() : 0;
  }
  virtual void setResultType(const Type* T) {
    if (functionType_) functionType_->setResultType(T);
  }
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<extern '" << name_
       << "' " << (functionType_ ? functionType_->toString(level+1) : "<no function type>")
       << '>';
    return ss.str();
  }
};

}} // namespace hue.ast
#endif // HUE__AST_PROTOTYPE_H
