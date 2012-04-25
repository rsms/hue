#ifndef RSMS_AST_TYPE_DECLARATION_H
#define RSMS_AST_TYPE_DECLARATION_H
#include <string>
#include <vector>
namespace rsms { namespace ast {

// Declares a type, e.g. Float (double precision number) or [Int] (list of integer numbers)
class TypeDeclaration {
public:
  enum Type {
    Unknown = 0,
    Named,
    Int,
    Float,
    Func,
  } type;
  
  TypeDeclaration(Type type) : type(type) {}
  TypeDeclaration(const std::string& name) : type(Named), name_(name) {}
  
  std::string toString() const {
    switch (type) {
      case Named: return name_;
      case Int: return "Int";
      case Float: return "Float";
      case Func: return "func";
      default: return "?";
    }
  }
private:
  std::string name_;
};

typedef std::vector<TypeDeclaration*> TypeDeclarationList;

}} // namespace rsms::ast
#endif  // RSMS_AST_TYPE_DECLARATION_H
