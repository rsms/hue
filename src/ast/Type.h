#ifndef RSMS_AST_TYPE_DECLARATION_H
#define RSMS_AST_TYPE_DECLARATION_H
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
    Func,
  };
  
  Type(TypeID typeID) : typeID_(typeID) {}
  Type(const std::string& name) : typeID_(Named), name_(name) {}
  
  inline const TypeID& typeID() const { return typeID_; }
  inline const std::string& name() const { return name_; }
  
  std::string toString() const {
    switch (typeID_) {
      case Named: return name_;
      case Int: return "Int";
      case Float: return "Float";
      case Func: return "func";
      default: return "?";
    }
  }
private:
  TypeID typeID_;
  std::string name_;
};

typedef std::vector<Type*> TypeList;

}} // namespace rsms::ast
#endif  // RSMS_AST_TYPE_DECLARATION_H
