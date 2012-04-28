#ifndef RSMS_AST_VARIABLE_DEFINITION_H
#define RSMS_AST_VARIABLE_DEFINITION_H
#include "Node.h"
#include "TypeDeclaration.h"
#include <vector>
#include <string>
namespace rsms { namespace ast {

class Variable : public Node {
public:
  Variable(bool isMutable, const std::string& name, TypeDeclaration *type)
    : isMutable_(isMutable), name_(name), type_(type) {}
  
  const bool& isMutable() const { return isMutable_; }
  const std::string& name() const { return name_; }
  
  TypeDeclaration *type() const { return type_; }
  bool hasUnknownType() const { return !type_ || type_->type == TypeDeclaration::Unknown; }
  
  void setType(TypeDeclaration *type) {
    TypeDeclaration *old = type_;
    type_ = type;
    if (old) delete old; 
  }
  
  std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Variable "
       << name_ << ' '
       << (isMutable_ ? "MUTABLE " : "const ")
       << (type_ ? type_->toString() : "?")
       << '>';
    return ss.str();
  }
private:
  bool isMutable_;
  std::string name_;
  TypeDeclaration *type_;
};

typedef std::vector<Variable*> VariableList;

}} // namespace rsms::ast
#endif  // RSMS_AST_VARIABLE_DEFINITION_H
