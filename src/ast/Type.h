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
    Nil, // == i1 0
    Named,
    Float,
    Int,
    Char,
    Byte,
    Bool,
    Array,
    StructureT,
    FuncT,

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
  inline bool isFunction() const { return typeID_ == FuncT; }
  inline bool isInt() const { return typeID_ == Int; }
  inline bool isFloat() const { return typeID_ == Float; }
  inline bool isStructure() const { return typeID_ == StructureT; }
  
  virtual std::string toString() const {
    switch (typeID_) {
      case Unknown: return "?";
      case Nil:     return "Nil";
      case Named:   return name_.UTF8String();
      case Float:   return "Float";
      case Int:     return "Int";
      case Char:    return "Char";
      case Byte:    return "Byte";
      case Bool:    return "Bool";
      case FuncT:    return "func";
      case StructureT: return "struct";
      case Array:   return "[?]";
      default:      return "";
    }
  }
  
  virtual std::string toHueSource() const { return toString(); }

  virtual std::string canonicalName() const { return toString(); }

  static const Type* highestFidelity(const Type* T1, const Type* T2);
  
private:
  TypeID typeID_;
  Text name_;
};

extern const Type UnknownType;
extern const Type NilType;
extern const Type FloatType;
extern const Type IntType;
extern const Type CharType;
extern const Type ByteType;
extern const Type BoolType;


class ArrayType : public Type {
  ArrayType(const Type* type) : Type(Array), type_(type) {}
public:
  static const ArrayType* get(const Type* elementType) {
    // TODO: reuse
    return new ArrayType(elementType);
  }

  static const ArrayType* get(const Type& elementType) {
    // TODO: reuse
    return get(&elementType);
  }

  inline const Type* type() const { return type_; }
  
  virtual std::string toString() const {
    std::string s("[");
    s += (type_ ? type_->toString() : "<nil>");
    s += "]";
    return s;
  }

  virtual std::string canonicalName() const {
    std::string s("vector");
    if (type_)
      s += '$' + type_->canonicalName();
    return s;
  }

private:
  const Type* type_;
};


typedef std::vector<const Type*> TypeList;

}} // namespace hue::ast
#endif  // HUE__AST_TYPE_DECLARATION_H
