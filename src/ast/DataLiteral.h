// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#ifndef HUE__AST_DATA_LITERAL_H
#define HUE__AST_DATA_LITERAL_H
#include "Expression.h"
#include <string>
#include <ctype.h>

namespace hue { namespace ast {

class DataLiteral : public Expression {
public:
  DataLiteral(const ByteString& data) : Expression(TDataLiteral, new ArrayType(new Type(Type::Byte))), data_(data) {}

  inline const ByteString& data() const { return data_; };

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<DataLiteral [" << data_.size() << "] '";
    ByteString::const_iterator it = data_.begin();
    
    for (; it != data_.end(); ++ it) {
      uint8_t c = *it;
      if (c == '\\') {
        ss << "\\\\";
      } else if (c == '\'') {
        ss << "\\'";
      } else if (Text::isPrintableASCII(c)) {
        ss << (char)c;
      } else {
        ss.flags(std::ios::hex);
        ss << (c < 0x10 ? "\\x0" : "\\x") << (uint16_t)c;
        ss.flags();
      }
    }
    
    ss << "'>";
    return ss.str();
  }
  
private:
  ByteString data_;
};

}} // namespace hue.ast
#endif // HUE__AST_DATA_LITERAL_H
