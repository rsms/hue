#include "../src/Mangle.h"
#include "../src/ast/Type.h"
#include "../src/ast/Variable.h"

#include <assert.h>
#include <stdlib.h>
#include <iostream>

using std::cout;
using std::endl;
using namespace hue;

int main() {
  ast::Type NamedT("Foo");
  ast::Type FloatT(ast::Type::Float);
  ast::Type IntT(ast::Type::Int);
  ast::Type CharT(ast::Type::Char);
  ast::Type ByteT(ast::Type::Byte);
  ast::Type BoolT(ast::Type::Bool);
  ast::Type FuncT(ast::Type::Func);
  
  // Verify Type -> Mangled id
  assert(hue::mangle(NamedT) == "N3Foo");
  assert(hue::mangle(FloatT) == "F");
  assert(hue::mangle(IntT)   == "I");
  assert(hue::mangle(CharT)  == "C");
  assert(hue::mangle(ByteT)  == "B");
  assert(hue::mangle(BoolT)  == "b");
  assert(hue::mangle(FuncT)  == "f");
  
  // Verify TypeList -> Mangled
  ast::TypeList types;
  types.push_back(&NamedT);
  types.push_back(&FloatT);
  types.push_back(&IntT);
  types.push_back(&CharT);
  types.push_back(&ByteT);
  types.push_back(&BoolT);
  types.push_back(&FuncT);
  assert(hue::mangle(types) == "N3FooFICBbf");
  
  // Verify VariableList -> Mangled
  ast::VariableList vars;
  vars.push_back(new ast::Variable(&IntT));
  vars.push_back(new ast::Variable(&IntT));
  vars.push_back(new ast::Variable(&FloatT));
  assert(hue::mangle(vars) == "IIF");
  
  // Verify ^(a,b Int) Int -> Mangled
  types.clear();
  types.push_back(&IntT);
  ast::FunctionType funcT1(&vars, &types);
  assert(hue::mangle(funcT1) == "$IIF$I");
  
  return 0;
}
