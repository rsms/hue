// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#ifndef HUE__AST_TYPE_DECLARATION_H
#define HUE__AST_TYPE_DECLARATION_H
#include "../Text.h"
#include <string>
#include <vector>

namespace hue { namespace ast {

class ArrayType;

// Declares a type, e.g. Float (double precision number) or [Int] (list of integer numbers)
class Type {
public:
  enum TypeID {
    Unknown = 0,
    None, // void
    Named,
    Float,
    Int,
    Char,
    Byte,
    Bool,
    Func,
    Array,

    MaxTypeID
  };
  
  Type(TypeID typeID) : typeID_(typeID) {}
  Type(const Text& name) : typeID_(Named), name_(name) {}
  Type(const Type& other) : typeID_(other.typeID_), name_(other.name_) {}
  virtual ~Type() {}
  
  inline const TypeID& typeID() const { return typeID_; }
  inline const Text& name() const { return name_; }
  
  bool isEqual(const Type& other) const {
    if (other.typeID() != typeID_) return false;
    return (typeID_ != Named || other.name() == name());
  }

  inline bool isUnknown() const { return typeID_ == Unknown; }
  inline bool isFunction() const { return typeID_ == Func; }
  inline bool isInt() const { return typeID_ == Int; }
  inline bool isFloat() const { return typeID_ == Float; }
  
  virtual std::string toString() const {
    switch (typeID_) {
      case Named: return name_.UTF8String();
      case Float: return "Float";
      case Int: return "Int";
      case Char: return "Char";
      case Byte: return "Byte";
      case Bool: return "Bool";
      case Func: return "func";
      default: return "?";
    }
  }
  
  virtual std::string toHueSource() const { return toString(); }

  static const Type* highestFidelity(const Type* T1, const Type* T2) {
    if (T1 == T2 || T1->isEqual(*T2)) {
      return T1;
    } else if (T1->isInt() && T2->isFloat()) {
      return T2;
    } else if (T2->isInt() && T1->isFloat()) {
      return T1;
    } else {
      // Unknown
      return 0;
    }
  }
  
private:
  TypeID typeID_;
  Text name_;
};

static const Type UnknownType(Type::Unknown);


class ArrayType : public Type {
public:
  ArrayType(Type* type) : Type(Array), type_(type) {}
  inline const Type* type() const { return type_; }
  
  virtual std::string toString() const {
    std::string s("[");
    s += (type_ ? type_->toString() : "<nil>");
    s += "]";
    return s;
  }
private:
  Type* type_;
};


typedef std::vector<Type*> TypeList;

}} // namespace hue::ast
#endif  // HUE__AST_TYPE_DECLARATION_H
