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
    Named,
    Float,
    Int,
    Char,
    Byte,
    Bool,
    Func,
    Array,
  };
  
  Type(TypeID typeID) : typeID_(typeID) {}
  Type(const Text& name) : typeID_(Named), name_(name) {}
  Type(const Type& other) : typeID_(other.typeID_), name_(other.name_) {}
  virtual ~Type() {}
  
  inline const TypeID& typeID() const { return typeID_; }
  inline const Text& name() const { return name_; }
  
  std::string toMangleID() const {
    if (typeID_ == Named) {
      std::string utf8name = name_.UTF8String();
      char buf[12];
      int len = snprintf(buf, 12, "N%zu", utf8name.length());
      utf8name.insert(0, buf, len);
      return utf8name;
    } else switch (typeID_) {
      case Float: return "F";
      case Int:   return "I";
      case Char:  return "C";
      case Byte:  return "B";
      case Bool:  return "b";
      case Func:  return "f";
      default:    return "";
    }
  }
  
  static Type createFromMangleID(const std::string& mangleID) {
    if (mangleID.length() == 1) {
      switch (mangleID[0]) {
        case 'F': return Type(Float);
        case 'I': return Type(Int);
        case 'C': return Type(Char);
        case 'B': return Type(Byte);
        case 'b': return Type(Bool);
        case 'f': return Type(Func);
        default:  return Type(Unknown);
      }
    } else {
      return Type(Text(mangleID));
    }
  }
  
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
private:
  TypeID typeID_;
  Text name_;
};


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
