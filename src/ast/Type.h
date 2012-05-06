#ifndef RSMS_AST_TYPE_DECLARATION_H
#define RSMS_AST_TYPE_DECLARATION_H
#include "../Text.h"
#include <string>
#include <vector>

namespace rsms { namespace ast {

// Declares a type, e.g. Float (double precision number) or [Int] (list of integer numbers)
class Type {
public:
  enum TypeID {
    Unknown = 0,
    Named,
    Int,
    Float,
    Bool,
    Func,
  };
  
  Type(TypeID typeID) : typeID_(typeID) {}
  Type(const Text& name) : typeID_(Named), name_(name) {}
  
  inline const TypeID& typeID() const { return typeID_; }
  inline const Text& name() const { return name_; }
  
  std::string toString() const {
    switch (typeID_) {
      case Named: return name_.UTF8String();
      case Int: return "Int";
      case Float: return "Float";
      case Bool: return "Bool";
      case Func: return "func";
      default: return "?";
    }
  }
private:
  TypeID typeID_;
  Text name_;
};

typedef std::vector<Type*> TypeList;

}} // namespace rsms::ast
#endif  // RSMS_AST_TYPE_DECLARATION_H
