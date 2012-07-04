// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "Type.h"

namespace hue { namespace ast {

const Type UnknownType(Type::Unknown);
const Type NilType(Type::Nil);
const Type FloatType(Type::Float);
const Type IntType(Type::Int);
const Type CharType(Type::Char);
const Type ByteType(Type::Byte);
const Type BoolType(Type::Bool);
//const Type FuncType(Type::Func);

const Type* Type::highestFidelity(const Type* T1, const Type* T2) {
  if (T1 && !T1->isUnknown() && T2 && !T2->isUnknown()) {
    if (T1 == T2 || T1->isEqual(*T2)) {
      return T1;
    } else if (T1->isInt() && T2->isFloat()) {
      return T2;
    } else if (T2->isInt() && T1->isFloat()) {
      return T1;
    }
  } else if (T1 && !T1->isUnknown()) {
    return T1;
  } else if (T2 && !T2->isUnknown()) {
    return T2;
  }
  return &UnknownType;
}

}} // namespace hue::ast
