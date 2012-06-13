// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

// A tokenizer produce tokens parsed from a ByteInput
#ifndef HUE__TOKENIZER_H
#define HUE__TOKENIZER_H

#include "../fastcmp.h"
#include "../Logger.h"
#include "../Text.h"

#include "ByteInput.h"
#include "Token.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

#include <assert.h>

namespace hue {


static inline uint32_t _strtouint32(const char *pch, int radix) {
  char *endptr = NULL;
  long val = strtol(pch, &endptr, radix);
  if (!endptr || val < 0) return 0;
  else if (val > 0xffffffffL) return 0xffffffff;
  return (uint32_t)val;
}


class Tokenizer {
  const Text& source_;
  uint32_t sourceOffset_;
  Token token_;
  uint32_t line_;
  uint32_t column_;
  uint32_t length_;
  long lineLeading_;
  bool hasStarted_;
  
public:

  Tokenizer(const Text& source)
      : source_(source)
      , sourceOffset_(0)
      , line_(1)
      , column_(1)
      , length_(0)
      , lineLeading_(0)
      , hasStarted_(false)
  {
    token_.type = Token::End;
  }
  
  const UChar& nextChar(size_t stride = 1) {
    column_ += stride;
    sourceOffset_ += stride;
    if (source_.size() < sourceOffset_) {
      sourceOffset_ = source_.size();
    }
    return source_[sourceOffset_];
  }
  
  inline const UChar& currentChar() const { return source_[sourceOffset_]; }
  inline const UChar& otherChar(size_t offset) const { return source_[sourceOffset_ + offset]; }
  inline size_t futureCharCount() const { return source_.size() - sourceOffset_; }
  inline bool atEnd() const { return source_.size() == sourceOffset_; }
  
  
  const Token& current() const {
    return token_;
  }
  
  uint32_t _parseHexLiteral(size_t maxLength) {
    size_t count = 0;
    const size_t bufsize = 9;
    assert(maxLength < bufsize);
    char buf[bufsize];
    
    while (!atEnd() && count != maxLength) {
      if (currentChar() == '_') {
        // ignore
      } else if (Text::isHexDigit(currentChar())) {
        buf[count++] = (char)(0x000000ff & currentChar());
      } else {
        break;
      }
      nextChar();
    }
    buf[count] = '\0';

    return _strtouint32(buf, 16);
  }
  
  inline bool _isIdChar(const UChar& c) const {
    return isalnum(c) || c == '_' || c > 0x7f;
  }
  
  void _parseTextOrDataLiteral(uint32_t startColumn, bool isText) {
    token_.line = line_;
    token_.column = startColumn;
    token_.textValue.clear();
    token_.type = isText ? Token::TextLiteral : Token::DataLiteral;

    bool escapeActive = false;
    const UChar delimiterChar = isText ? '"' : '\'';
    const UChar escapeStartChar = '\\';
    size_t count = 0;
    
    nextChar(); // Eat delimiterChar
    
    while (!atEnd()) {
      
      // Allocate space in chunks
      if (token_.textValue.capacity() < count + 32) {
        token_.textValue.reserve(count + 256);
      }

      // In an escape sequence?
      if (escapeActive) {
        escapeActive = false;
        
        switch (currentChar()) {
          case 't': token_.textValue.append(1, (UChar)9); break;
          case 'n': token_.textValue.append(1, (UChar)10); break;
          case 'r': token_.textValue.append(1, (UChar)12); break;
          case escapeStartChar: token_.textValue.append(1, escapeStartChar); break;
          case '0': token_.textValue.append(1, (UChar)0); break;
          default: {
            if (currentChar() == delimiterChar) {
              token_.textValue.append(1, delimiterChar); break;

            } else if (Text::isWhitespaceOrLineSeparator(currentChar())) {
              // eat and ignore whitespace

            } else if (isText && currentChar() == 'u') {
              // Read up to 8 digits (32-bit Unicode value).
              nextChar(); // eat 'u'
              token_.textValue += _parseHexLiteral(8);
              continue; // to avoid nextChar() since we have already advanced

            } else if (!isText && currentChar() == 'x') {
              nextChar(); // eat 'x'
              UChar value = _parseHexLiteral(2);
              assert((0x000000ff & value) == value);
              token_.textValue += value;
              continue; // to avoid nextChar() since we have already advanced

            } else {
              token_.type = Token::Error;
              token_.textValue = isText ? "Invalid escape sequence in text literal"
                                        : "Invalid escape sequence in data literal";
              goto end_of_reading_sequence;
            }
          }
        }

      } else if (currentChar() == escapeStartChar) {
        // Start of escape sequence
        escapeActive = true;

      } else if (currentChar() == delimiterChar) {
        // delimiter (" or ') w/o being part of an esc sequence means end of literal
        nextChar(); // consume delimiter
        break;

      } else {
        token_.textValue += currentChar();
      }
      
      nextChar();
    }
    end_of_reading_sequence:

    token_.length = column_ - startColumn - 1;
  }
  
  const Token& next() {

    // First time we need to start the input; produce a NewLine
    if (hasStarted_ == false) {
      hasStarted_ = true;
      token_.line = 1;
      token_.column = 0;
      token_.length = 0;
      token_.type = Token::NewLine;
      return token_;
    }
    
    // Skip any whitespace.
    if (Text::isWhitespaceOrLineSeparator(currentChar())) {
      uint32_t lengthAfterLF = 0;
      bool afterLF = false;
      
      do {
        if (currentChar() == 10) { // LF
          if (!afterLF) {
            afterLF = true;
          }
          lengthAfterLF = 0;
          ++line_;
          column_ = 1;
        } else if (afterLF) {
          ++lengthAfterLF;
        }
      } while (Text::isWhitespaceOrLineSeparator(nextChar()));
      
      if (afterLF) {
        token_.line = line_;
        token_.column = 0;
        token_.length = lengthAfterLF;
        token_.type = Token::NewLine;
        return token_;
      }
    }
    
    uint32_t startColumn = column_-1;
    
    // End
    if (atEnd()) {
      token_.line = line_;
      token_.column = column_;
      token_.type = Token::End;
    }
    
    // IntegerHexLiteral = '0x' (0..9 | A..F | a..f | _)+
    else if (currentChar() == '0' && futureCharCount() > 1 && otherChar(1) == 'x') {
      nextChar(2); // eat '0','x'
      token_.textValue.clear();
      token_.intValue = 16; // radix
      
      while (!atEnd()) {
        if (currentChar() == '_') {
          // ignore
        } else if (Text::isHexDigit(currentChar())) {
          token_.textValue += currentChar();
        } else {
          break;
        }
        nextChar();
      }
      
      token_.type = Token::IntLiteral;
      token_.line = line_;
      token_.column = startColumn;
      token_.length = column_ - startColumn - 1;
    }
    
    // IntegerDecLiteral = (0..9 | _)+
    // FloatLiteral = (0..9 | _)+
    else if (   Text::isDecimalDigit(currentChar())
             || (currentChar() == '.' && futureCharCount() != 0 && Text::isDecimalDigit(otherChar(1))) ) {
      token_.type = currentChar() == '.' ? Token::FloatLiteral : Token::IntLiteral;
      token_.textValue = currentChar();
      token_.intValue = 10; // radix
      bool lastCharWasDot = false;
      bool seenDot = false;
      
      while (1) {
        nextChar();
        if (currentChar() == '_') {
          continue; // ignore
        }
        
        if (currentChar() == '.') {
          if (seenDot) {
            break;
          } else {
            token_.type = Token::FloatLiteral;
            token_.textValue += currentChar();
            seenDot = lastCharWasDot = true;
          }
          continue;
        } else if (currentChar() == 'E' || currentChar() == 'e') {
          // E+1, e+1, E1
          lastCharWasDot = false;
          token_.type = Token::FloatLiteral;
          if (futureCharCount() != 0 && Text::isDecimalDigit(otherChar(1)) ) {
            token_.textValue += currentChar();
            token_.textValue += nextChar();
            continue;
          } else if (   futureCharCount() > 1
                     && (otherChar(1) == '+' || otherChar(1) == '-')
                     && Text::isDecimalDigit(otherChar(2)) ) {
            token_.textValue += currentChar();
            token_.textValue += nextChar();
            token_.textValue += nextChar();
            continue;
          } else {
            token_.type = Token::Error;
            token_.textValue = "Expected '+', '-' or none, followed by a decimal digit "
                               "after exponent in floating point number literal";
            goto return_token;
          }
        } else if (!Text::isDecimalDigit(currentChar())) {
          break;
        }
        
        token_.textValue += currentChar();
        lastCharWasDot = false;
      }
      
      if (lastCharWasDot) {
        token_.type = Token::Error;
        token_.textValue = "Unexpected '.' at end of floating point number literal";
      }

      token_.line = line_;
      token_.column = startColumn;
      token_.length = column_ - startColumn - 1;
    }
    
    // Text literal
    else if (currentChar() == '"') {
      _parseTextOrDataLiteral(startColumn, /* isText = */true);
    }
    
    // Data literal
    else if (currentChar() == '\'') {
      _parseTextOrDataLiteral(startColumn, /* isText = */false);
    }
    
    // Comment until end of line: #.*
    else if (currentChar() == '#') {
      token_.line = line_;
      token_.column = startColumn;
      token_.type = Token::Comment;
      token_.textValue = currentChar();
      while (nextChar() != InputEnd && currentChar() != '\n' && currentChar() != '\r') {
        token_.textValue += currentChar();
      }
      token_.length = column_ - startColumn - 1;
      
      // This code ignores the comment instead of producing a token
      // while (nextChar() != InputEnd && currentChar() != '\n' && currentChar() != '\r');
      // if (currentChar() != InputEnd) {
      //   goto start_of_next; // return next()
      // }
    }
    
    // Simple 1-2 byte tokens
    else {
      // 2-byte equality operator: '!=' '<=' '>=' '=='
      if ( (futureCharCount() > 0 && otherChar(1) == '=') && (
                currentChar() == '!'
             || currentChar() == '<'
             || currentChar() == '>'
             || currentChar() == '='
           ) )
      {
        token_.textValue = currentChar();
        token_.textValue += '='; //otherChar(1);
        token_.line = line_;
        token_.column = column_;
        token_.length = 2;
        token_.type = Token::BinaryComparisonOperator;
        //rlog("BinaryComparisonOperator: " << token_.toString());
        nextChar(); // consume
      }
      
      // Single-byte tokens
      else {
        bool shouldParseIdentifier = false;
        token_.line = line_;
        token_.column = column_-1;
        token_.length = 1;
        
        switch (currentChar()) {
          case '<': { // '<-'?
            if (futureCharCount() > 0 && otherChar(1) == '-') {
              token_.type = Token::LeftArrow;
              token_.length = 2;
              nextChar(); // consume '-'
              break;
            }
          }
          case '-': { // '->'?
            if (futureCharCount() > 0 && otherChar(1) == '>') {
              token_.type = Token::RightArrow;
              token_.length = 2;
              nextChar(); // consume '>'
              break;
            }
          }
          case '>':
          case '+':
          case '*':
          //case '=':
          case '/': token_.type = Token::BinaryOperator; break;
          
          case ':': token_.type = Token::Colon; break;
          case ';': token_.type = Token::Semicolon; break;
          case '?': token_.type = Token::Question; break;
          case '\\':token_.type = Token::Backslash; break;
          case '=': token_.type = Token::Assignment; break;
          case '(': token_.type = Token::LeftParen; break;
          case ')': token_.type = Token::RightParen; break;
          case '[': token_.type = Token::LeftSqBracket; break;
          case ']': token_.type = Token::RightSqBracket; break;
          case ',': token_.type = Token::Comma; break;
          case '.': token_.type = Token::Stop; break;
          //case '^': token_.type = Token::Func; break;

          case '{':
          case '}': token_.type = Token::MapLiteral; break;
        
          default: {
            shouldParseIdentifier = true;
            break;
          }
        }
        
        if (shouldParseIdentifier) {
          
          // Parse identifier
          if ( _isIdChar(currentChar()) ) {
            const UChar *datav = source_.data() + sourceOffset_;
    
            #define CONSUME_SYMBOL(TYPE, LEN) do {\
              token_.line = line_; \
              token_.column = startColumn; \
              token_.length = LEN; \
              token_.type = Token::TYPE; \
              nextChar(LEN); } while(0)
            #define _i32CMP(LEN, ...) (futureCharCount() >= LEN && hue_i32cmp##LEN(datav, __VA_ARGS__))
            #define i32CMP_then_CONSUME_SYMBOL(TYPE, LEN, ...) (_i32CMP(LEN, __VA_ARGS__)) CONSUME_SYMBOL(TYPE, LEN)

                 if i32CMP_then_CONSUME_SYMBOL(If,           2, 'i','f');
            else if i32CMP_then_CONSUME_SYMBOL(Else,         4, 'e','l','s','e');
            else if i32CMP_then_CONSUME_SYMBOL(Func,         4, 'f','u','n','c');
            else if i32CMP_then_CONSUME_SYMBOL(External,     6, 'e','x','t','e','r','n');
            else if i32CMP_then_CONSUME_SYMBOL(Nil,          3, 'n','i','l');
            else if i32CMP_then_CONSUME_SYMBOL(Bool,         4, 'B','o','o','l');
            else if i32CMP_then_CONSUME_SYMBOL(IntSymbol,    3, 'I','n','t');
            else if i32CMP_then_CONSUME_SYMBOL(FloatSymbol,  5, 'F','l','o','a','t');
            else if i32CMP_then_CONSUME_SYMBOL(Byte,         4, 'B','y','t','e');
            else if i32CMP_then_CONSUME_SYMBOL(Char,         4, 'C','h','a','r');
            else if i32CMP_then_CONSUME_SYMBOL(Structure,    6, 's','t','r','u','c','t');
            else if i32CMP_then_CONSUME_SYMBOL(Mutable,      7, 'M','U','T','A','B','L','E');
    
            else if (_i32CMP(4, 't','r','u','e')) {
              token_.line = line_;
              token_.column = startColumn;
              token_.length = 4;
              token_.intValue = 1;
              token_.type = Token::BoolLiteral;
              nextChar(4);
            }
            else if (_i32CMP(5, 'f','a','l','s','e')) {
              token_.line = line_;
              token_.column = startColumn;
              token_.length = 5;
              token_.intValue = 0;
              token_.type = Token::BoolLiteral;
              nextChar(5);
            }
    
            else {
              token_.line = line_;
              token_.column = startColumn;
              token_.length = 1;
              token_.type = Token::Identifier;
              token_.intValue = 0;
              token_.textValue = currentChar();
              while (1) {
                if (nextChar() == ':') {
                  token_.intValue |= (uint8_t)Token::Flag_Path; // OPT: means that the symbol is a path
                } else if (currentChar() == '/') {
                  token_.intValue |= (uint8_t)Token::Flag_Namespaced; // OPT: means that the symbol is namespaced
                } else if (!_isIdChar(currentChar())) {
                  break;
                }
                token_.textValue += currentChar();
              }
              token_.length = token_.textValue.size();
            }
    
            #undef if_i8CMP_then_CONSUME_SYMBOL
            #undef if_i32CMP
            #undef CONSUME_SYMBOL
          
            goto return_token; // to avoid an extra nextChar() since we already advanced

          } else {
            token_.type = Token::Error;
            std::ostringstream ss;
            ss << "Unexpected character: ";
            if (Text::isPrintableASCII(currentChar())) {
              ss << '\'' << (char)(0x000000ff & currentChar()) << '\'';
            } else {
              ss << "\\u";
              ss.flags(std::ios::hex);
              ss << currentChar();
            }
            token_.textValue = ss.str();
          }
          
        } else {
          if (Token::TypeInfo[token_.type].hasTextValue) {
            token_.textValue = currentChar();
          }
          //token_.line = line_;
          //token_.column = column_;
        }
      }
        
      nextChar(); // consume
    }
    
    return_token:
    
    return token_;
  }

};


} // namespace hue

#endif // HUE__TOKENIZER_H
