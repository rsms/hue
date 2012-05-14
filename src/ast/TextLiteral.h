// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#ifndef HUE__AST_TEXT_LITERAL_H
#define HUE__AST_TEXT_LITERAL_H
#include "Expression.h"
#include "../utf8/checked.h"

namespace hue { namespace ast {

class TextLiteral : public Expression {
public:
  TextLiteral(const Text& text) : Expression(TTextLiteral), text_(text) {}
  const Text& text() const { return text_; }

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<TextLiteral [" << text_.size() << "] \"";
    Text::const_iterator it = text_.begin();
    
    for (; it != text_.end(); ++ it) {
      UChar c = *it;
      if (c == '\\') {
        ss << "\\\\";
      } else if (c == '"') {
        ss << "\\\"";
      } else if (c == 0x0a) {
        ss << "\\n";
      } else if (c == 0x0d) {
        ss << "\\r";
      } else if (c == 0x09) {
        ss << "\\t";
      } else if (Text::isPrintable(c) || Text::isWhitespace(c)) {
        ss << Text::UCharToUTF8String(c);
      } else {
        ss.flags(std::ios::hex);
        ss << "\\u" << c;
        ss.flags();
      }
    }
    
    ss << "\">";
    return ss.str();
  }
  
private:
  Text text_;
};

}} // namespace hue.ast
#endif // HUE__AST_TEXT_LITERAL_H
