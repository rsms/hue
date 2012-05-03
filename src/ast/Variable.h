#ifndef RSMS_AST_VARIABLE_DEFINITION_H
#define RSMS_AST_VARIABLE_DEFINITION_H
#include "Node.h"
#include "Type.h"
#include <vector>
#include <string>
namespace rsms { namespace ast {

class Variable : public Node {
public:
  Variable(bool isMutable, const std::string& name, Type *type)
    : isMutable_(isMutable), name_(name), type_(type) {}
  
  const bool& isMutable() const { return isMutable_; }
  const std::string& name() const { return name_; }
  
  Type *type() const { return type_; }
  bool hasUnknownType() const { return !type_ || type_->typeID() == Type::Unknown; }
  void setType(Type *type) { type_ = type; }
  
  std::string toString(int level = 0) const {
    std::ostringstream ss;
    //NodeToStringHeader(level, ss);
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
  Type *type_;
};

typedef std::vector<Variable*> VariableList;

}} // namespace rsms::ast
#endif  // RSMS_AST_VARIABLE_DEFINITION_H
