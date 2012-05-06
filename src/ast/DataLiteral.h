#ifndef RSMS_AST_DATA_LITERAL_H
#define RSMS_AST_DATA_LITERAL_H
#include "Expression.h"
#include <string>
#include <ctype.h>

namespace rsms { namespace ast {

class DataLiteral : public Expression {
public:
  DataLiteral(const std::string& data) : Expression(TConditional), data_(data) {}

  virtual std::string toString(int level = 0) const {
    std::ostringstream ss;
    NodeToStringHeader(level, ss);
    ss << "<DataLiteral [" << data_.size() << "] '";
    std::string::const_iterator it = data_.begin();
    ss.flags(std::ios::hex);
    for (; it != data_.end(); ++ it) {
      UChar c = *it;
      if (c == '\\') {
        ss << "\\\\";
      } else if (Text::isPrintableASCII(c)) {
        ss << c;
      } else {
        ss << "\\u" << (char)(0x000000ff & c);
      }
    }
    ss.flags();
    ss << "'>";
    return ss.str();
  }
  
private:
  Text data_;
};

}} // namespace rsms.ast
#endif // RSMS_AST_DATA_LITERAL_H
