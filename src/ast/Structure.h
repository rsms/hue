// Copyright (c) 2012, Rasmus Andersson. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.
#ifndef HUE_AST_STRUCTURE_H
#define HUE_AST_STRUCTURE_H
#include "StructType.h"
#include "Expression.h"
#include "Block.h"
#include <assert.h>
#include <vector>
#include <map>

namespace hue { namespace ast {

class Structure : public Expression {
public:
  class Member {
  public:
    Member() : index(0), value(0) {}
    size_t index;
    Expression* value;
  };

  typedef std::map<const Text, Member> MemberMap;

  Structure(Block* block = 0) : Expression(TStructure), block_(0), structType_(0) {
    setBlock(block);
  }

  Block* block() const { return block_; }
  void setBlock(Block* B) {
    block_ = B;
    update();
  }

  void update();

  const StructType *resultStructType() const { return structType_; }
  virtual const Type *resultType() const { return resultStructType(); }
  virtual void setResultType(const Type* T) throw(std::logic_error) {
    throw std::logic_error("Can not set type for 'struct' expression");
  }

  // Get member for name (or nil if not found)
  const Member* operator[](const Text& name) const;

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    ss << "(struct ";
    NodeToStringHeader(level+1, ss);
    ss << (block_ ? block_->toString(level+1) : "nil")
       << ")";
    return ss.str();
  }
  
private:
  Block* block_;
  StructType* structType_;
  MemberMap members_;
};

}} // namespace hue.ast
#endif // HUE_AST_STRUCTURE_H
