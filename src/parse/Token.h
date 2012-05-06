// Represents the simplest parts of the language: lexical tokens
#ifndef RSMS_TOKEN_H
#define RSMS_TOKEN_H

#include "../Text.h"
#include <string>

namespace rsms {

typedef struct {
  const char *name;
  const bool hasTextValue;
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
    BinaryOperator,  // '+', '*', '>', etc.
    BinaryComparisonOperator, // '!=' '<=' '>=' '=='. First byte is key.
    Structure,
    RightArrow, // '->'
    LeftArrow, // '<-'
    LeftParen, // '('
    RightParen, // ')'
    Comma, // ','
    Stop, // '.'
    Assignment, // '='
    Backslash, // '\'
    Question, // '?'
    Colon, // ':'
    Semicolon, // ';'
    NewLine,
    
    // Literals
    IntLiteral,
    FloatLiteral,
    BoolLiteral,    // intValue denotes value: 0 = false, !0 = true
    TextLiteral,    // textValue is the data
    DataLiteral,    // textValue is the data
    // TODO: BinaryLiteral
    
    // Symbols
    None,         // 'none'
    IntSymbol,    // 'Int'
    FloatSymbol,  // 'Float'
    Bool,         // 'Bool'
    If,           // 'if'
    Else,         // 'else'
    
    // Special meaning
    Error,        // textValue = message, intValue = code
    End,
    
    _TypeCount,
  } type;
  // The above enum MUST be aligned with the following array:
  static const TokenTypeInfo TypeInfo[_TypeCount];
  
  Text textValue;
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
      if (info.hasTextValue) textValue = other.textValue;
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
      if (info.hasTextValue) {
        return_fstr("%s@%u:%u,%u = %s", info.name, line, column, length, textValue.UTF8String().c_str());
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
  // name                // hasTextValue  hasDoubleValue  hasIntValue
  {"Unexpected",          .hasTextValue = 1, 0,0},
  {"Comment",             .hasTextValue = 1, 0,0},
  {"Func",                0,0,0},
  {"External",            0,0,0},
  {"Mutable",             0,0,0},
  {"Identifier",          .hasTextValue = 1, 0,0},
  {"BinaryOperator",      .hasTextValue = 1, 0,0},
  {"BinaryComparisonOperator",  .hasTextValue = 1, 0,0},
  {"Structure",           .hasTextValue = 1, 0,0},
  {"RightArrow",          0,0,0},
  {"LeftArrow",           0,0,0},
  {"LeftParen",           0,0,0},
  {"RightParen",          0,0,0},
  {"Comma",               0,0,0},
  {"Stop",                0,0,0},
  {"Assignment",          0,0,0},
  {"Backslash",           0,0,0},
  {"Question",            0,0,0},
  {"Colon",               0,0,0},
  {"Semicolon",           0,0,0},
  {"NewLine",             0,0,0},
  
  {"IntLiteral",          .hasTextValue = 1,0, .hasIntValue = 1}, // intValue = radix
  {"FloatLiteral",        .hasTextValue = 1,0,0},
  {"BoolLiteral",         0,0, .hasIntValue = 1}, // intValue = !0
  {"TextLiteral",         .hasTextValue = 1,0,0},
  {"DataLiteral",         .hasTextValue = 1,0,0},
  
  {"Nil",                 0,0,0},
  {"IntSymbol",           0,0,0},
  {"FloatSymbol",         0,0,0},
  {"Bool",                0,0,0},
  {"If",                  0,0,0},
  {"Else",                0,0,0},
  
  {"Error",               .hasTextValue = 1,0, .hasIntValue = 1},
  {"End",                 0,0,0},
};

} // namespace rsms

#endif // RSMS_TOKEN_H
