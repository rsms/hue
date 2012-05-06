#ifndef RSMS_AST_TEXT_LITERAL_H
#define RSMS_AST_TEXT_LITERAL_H
#include "Expression.h"
#include "../utf8/checked.h"

namespace rsms { namespace ast {

class TextLiteral : public Expression {
public:
  TextLiteral(const Text& text) : Expression(TTextLiteral), text_(text) {}

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

}} // namespace rsms.ast
#endif // RSMS_AST_TEXT_LITERAL_H
