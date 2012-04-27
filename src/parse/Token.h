// Represents the simplest parts of the language: lexical tokens
#ifndef RSMS_TOKEN_H
#define RSMS_TOKEN_H

#include <string>

namespace rsms {

typedef struct {
  const char *name;
  const bool hasStringValue;
  const bool hasDoubleValue;
  const bool hasIntValue;
} TokenTypeInfo;

class Token {
public:
  enum Type {
    Unexpected = 0,
    Comment, // #.+
    Func, // "func" ...
    External, // "extern"
    Mutable, // "MUTABLE"
    Identifier,
    BinaryOperator,
    ComparisonOperator,
    Structure,
    RightArrow, // '->'
    LeftArrow, // '<-'
    LeftParen, // '('
    RightParen, // ')'
    Comma, // ','
    Stop, // '.'
    Assignment, // '='
    NewLine,
    
    // Literals
    IntLiteral,
    FloatLiteral,
    
    // Built-in types
    IntSymbol,
    FloatSymbol,
    
    End,
    
    _TypeCount,
  } type;
  // The above enum MUST be aligned with the following array:
  static const TokenTypeInfo TypeInfo[_TypeCount];
  
  std::string stringValue;
  union {
    double doubleValue;
    uint8_t intValue;
  };
  
  uint32_t line;
  uint32_t column;
  uint32_t length;
  
  Token(Type type = _TypeCount) : type(type), line(0), column(0), length(0) {}
  Token(const Token &other) {
    type = other.type;
    line = other.line;
    column = other.column;
    length = other.length;
    // Value
    if (type >= 0 && type < _TypeCount) {
      const TokenTypeInfo& info = TypeInfo[type];
      if (info.hasStringValue) stringValue = other.stringValue;
      if (info.hasDoubleValue) doubleValue = other.doubleValue;
      if (info.hasIntValue)     intValue = other.intValue;
    }
  }
  
  bool isNull() const { return type == _TypeCount; }
  
  const char *typeName() const {
    if (type >= 0 && type < _TypeCount) {
      return TypeInfo[type].name;
    } else {
      return "?";
    }
  }
  
  std::string toString() const {
    #define return_fstr(fmt, ...) do { \
      const size_t bufsize = 512; \
      char buf[bufsize+1]; \
      snprintf(buf, bufsize, fmt, __VA_ARGS__); \
      return std::string(buf); \
    } while (0)
    
    if (type >= 0 && type < _TypeCount) {
      const TokenTypeInfo& info = TypeInfo[type];
      if (info.hasStringValue) {
        return_fstr("%s@%u:%u,%u = '%s'", info.name, line, column, length, stringValue.c_str());
      } else if (info.hasDoubleValue) {
        return_fstr("%s@%u:%u,%u = %f", info.name, line, column, length, doubleValue);
      } else if (info.hasIntValue) {
        return_fstr("%s@%u:%u,%u = %d", info.name, line, column, length, intValue);
      } else {
        return_fstr("%s@%u:%u,%u", info.name, line, column, length);
      }
    } else {
      return "?";
    }
  }
};

static const Token NullToken;

const TokenTypeInfo Token::TypeInfo[] = {
  // name                // hasStringValue  hasDoubleValue  hasIntValue
  {"Unexpected",          .hasStringValue = 1, 0,0},
  {"Comment",             .hasStringValue = 1, 0,0},
  {"Func",                0,0,0},
  {"External",            0,0,0},
  {"Mutable",             0,0,0},
  {"Identifier",          .hasStringValue = 1, 0,0},
  {"BinaryOperator",      .hasStringValue = 1, 0,0},
  {"ComparisonOperator",  .hasStringValue = 1, 0,0},
  {"Structure",           .hasStringValue = 1, 0,0},
  {"RightArrow",          0,0,0},
  {"LeftArrow",           0,0,0},
  {"LeftParen",           0,0,0},
  {"RightParen",          0,0,0},
  {"Comma",               0,0,0},
  {"Stop",                0,0,0},
  {"Assignment",          0,0,0},
  {"NewLine",             0,0,0},
  {"IntLiteral",          .hasStringValue = 1,0, .hasIntValue = 1}, // intValue = radix
  {"FloatLiteral",        0, .hasDoubleValue = 1,0},
  {"IntSymbol",           0,0,0},
  {"FloatSymbol",         0,0,0},
  {"End",                 0,0,0},
};

} // namespace rsms

#endif // RSMS_TOKEN_H
