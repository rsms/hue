// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#ifndef HUE__AST_PROTOTYPE_H
#define HUE__AST_PROTOTYPE_H
#include "Node.h"
#include "Expression.h"
#include "Type.h"
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

// Represents the "prototype" for a function, which captures its name, and its
// argument names (thus implicitly the number of arguments the function takes).
class FunctionType : public Node {
public:
  FunctionType(VariableList *args = 0,
               Type *returnType = 0,
               bool isPublic = false)
    : Node(TFunctionType), args_(args), returnType_(returnType), isPublic_(isPublic) {}

  VariableList *args() const { return args_; }

  Type *returnType() const { return returnType_; }
  void setReturnType(Type* T) {
    if (returnType_ != T) {
      Type* OT = returnType_;
      returnType_ = T;
      if (OT) delete OT;
    }
  }
  
  bool isPublic() const { return isPublic_; }
  void setIsPublic(bool isPublic) { isPublic_ = isPublic; }
  

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<FunctionType (";
    
    if (args_) {
      VariableList::const_iterator it1;
      it1 = args_->begin();
      if (it1 < args_->end()) { ss << (*it1)->toString(); it1++; }
      for (; it1 < args_->end(); it1++) { ss << ", " << (*it1)->toString(); }
    }
    
    ss << ")";
    if (returnType_) ss << ' ' << returnType_->toString();
    ss << '>';
    return ss.str();
  }
  
  
  virtual std::string toHueSource() const {
    std::ostringstream ss;
    ss << "^";
    
    if (args_ && !args_->empty()) {
      ss << '(';
      VariableList::const_iterator it1;
      it1 = args_->begin();
      for (; it1 != args_->end();) {
        ss << (*it1)->type()->toHueSource();
        ++it1;
        if (it1 != args_->end()) ss << ", ";
      }
      ss << ')';
    }
    
    if (returnType_)
      ss << ' ' << returnType_->toHueSource();
    
    return ss.str();
  }
  
  
private:
  VariableList *args_;
  Type *returnType_;
  bool isPublic_;
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

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Function "
       << (functionType_ ? functionType_->toString(level+1) : "<null>")
       << " -> "
       << (body_ ? body_->toString(level+1) : "<null>")
       << '>';
    return ss.str();
  }
};

// Represents an external function declaration.
class ExternalFunction : public Expression {
  Text name_;
  FunctionType *functionType_;
public:
  ExternalFunction(const Text &name, FunctionType *functionType)
    : Expression(TExternalFunction), name_(name), functionType_(functionType) {}
  
  inline const Text& name() const { return name_; }
  inline FunctionType *functionType() const { return functionType_; }
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<ExternalFunction name='" << name_
       << "' " << (functionType_ ? functionType_->toString(level+1) : "?")
       << '>';
    return ss.str();
  }
};

}} // namespace hue.ast
#endif // HUE__AST_PROTOTYPE_H
