// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef HUE_AST_STRUCT_TYPE_H
#define HUE_AST_STRUCT_TYPE_H

#include "Type.h"
#include <string>
#include <vector>
#include <map>

namespace hue { namespace ast {

class Block;
class FunctionType;

// A struct type represents an ordered map where the offset of a member
// signifies the offset into the underlying struct. The name is the name
// of the member and is primarily used for debugging.

// struct
//   foo = 1
//   bar = 2.3
// -->
// StructType
//   types_[Int, Float]
//   names_["foo", "bar"]
//

class StructType : public Type {
  typedef std::map<const Text, size_t> NameMap;
  StructType() : Type(StructureT) {}

public:
  class Member {
  public:
    typedef std::vector<Member> List;
    
    Member() : type(0), name() {}
    Member(const Type* T, const Text N, unsigned i) : type(T), name(N), index(i) {}
    //Member(const Member& other) : type(other.type), ... , name(other.name) {}

    const Type* type;
    Text name;
    unsigned index;
    
    // inline Member& operator= (const Member& rhs) {
    //   rhs
    //   return *this;
    // }
  };

  //static StructType* get(const TypeList& types);
  static StructType* get(const Member::List& members);

  inline size_t size() const { return types_.size(); }
  const TypeList& types() const { return types_; }
  bool isPacked() const { return true; }

  virtual std::string toString() const;
  virtual std::string canonicalName() const;

  // Struct offset for member with name. Returns SIZE_MAX if not found.
  size_t indexOf(const Text& name) const;

  // Get type for name (or nil if not found)
  const Type* operator[](const Text& name) const;

private:
  TypeList types_;
  NameMap nameToIndexMap_;
};

}} // namespace hue::ast
#endif  // HUE_AST_STRUCT_TYPE_H
