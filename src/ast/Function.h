#ifndef RSMS_AST_PROTOTYPE_H
#define RSMS_AST_PROTOTYPE_H
#include "Node.h"
#include "Expression.h"
#include "TypeDeclaration.h"
#include "Block.h"

namespace rsms { namespace ast {

class Argument {
public:
  std::string identifier;
  TypeDeclaration type;
  
  Argument(std::string identifier, const TypeDeclaration& type)
    : identifier(identifier), type(type) {}

  Argument(std::string identifier) : identifier(identifier), type(TypeDeclaration::Unknown) {}
  
  std::string toString() const {
    return identifier + " " + type.toString();
  }
};

// Represents the "prototype" for a function, which captures its name, and its
// argument names (thus implicitly the number of arguments the function takes).
class FunctionInterface : public Node {
  VariableList *args_;
  TypeDeclarationList *returnTypes_;
public:
  FunctionInterface(VariableList *args = 0, TypeDeclarationList *returnTypes = 0)
    : Node(TFunctionInterface), args_(args), returnTypes_(returnTypes) {}

  VariableList *args() const { return args_; }
  TypeDeclarationList *returnTypes() const { return returnTypes_; }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<FunctionInterface (";
    
    if (args_) {
      VariableList::const_iterator it1;
      it1 = args_->begin();
      if (it1 < args_->end()) { ss << (*it1)->toString(); it1++; }
      for (; it1 < args_->end(); it1++) { ss << ", " << (*it1)->toString(); }
    }
    
    ss << ")";
    
    if (returnTypes_) {
      ss << ' ';
      TypeDeclarationList::const_iterator it2;
      it2 = returnTypes_->begin();
      if (it2 < returnTypes_->end()) { ss << (*it2)->toString(); it2++; }
      for (; it2 < returnTypes_->end(); it2++) { ss << ", " << (*it2)->toString(); }
    }
    
    ss << '>';
    return ss.str();
  }
};

// Represents a function definition.
class Function : public Expression {
  FunctionInterface *interface_;
  Block *body_;
public:
  Function(FunctionInterface *interface, Block *body)
    : Expression(TFunction), interface_(interface), body_(body) {}
  Function(FunctionInterface *interface, Expression *body)
    : Expression(TFunction), interface_(interface), body_(new Block(body)) {}

  FunctionInterface *interface() const { return interface_; }
  Block *body() const { return body_; }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<Function "
       << (interface_ ? interface_->toString(level+1) : "<null>")
       << " -> "
       << (body_ ? body_->toString(level+1) : "<null>")
       << '>';
    return ss.str();
  }
};

// Represents an external function declaration.
class ExternalFunction : public Expression {
  std::string name_;
  FunctionInterface *interface_;
public:
  ExternalFunction(const std::string &name, FunctionInterface *interface)
    : Expression(TExternalFunction), name_(name), interface_(interface) {}
  
  const std::string& name() const { return name_; }
  FunctionInterface *interface() const { return interface_; }
  
  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<ExternalFunction name='" << name_ 
       << "' interface=" << (interface_ ? interface_->toString(level+1) : "<null>")
       << '>';
    return ss.str();
  }
};

}} // namespace rsms.ast
#endif // RSMS_AST_PROTOTYPE_H
