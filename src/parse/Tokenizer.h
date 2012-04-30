// A tokenizer produce tokens parsed from a ByteInput
#ifndef RSMS_TOKENIZER_H
#define RSMS_TOKENIZER_H

#include <iostream>
#include <fstream>

#include "../fastcmp.h"
#include "../Logger.h"
#include "ByteInput.h"
#include "Token.h"

namespace rsms {

inline static bool IsWhitespaceByte(const uint8_t& b) {
  return b == '\x20' || b == '\x09' || b == '\x0d' || b == '\x0a'; // SP || TAB || CR || LF
}

inline static bool IsNewLineByte(const uint8_t& b) {
  return b == '\x0a'; // LF
}

class Tokenizer {
  ByteInput *input_;
  Token token_;
  uint32_t line_;
  uint32_t column_;
  uint32_t length_;
  long lineLeading_;
  
public:
  
  Tokenizer(ByteInput *input)
      : input_(input)
      , line_(1)
      , column_(1)
      , length_(0)
      , lineLeading_(0)
  {
    token_.type = Token::End;
  }
  
  const uint8_t& nextByte(const size_t stride = 1) {
    column_ += stride;
    return input_->next(stride);
  }
  
  const Token& current() const {
    return token_;
  }
  
  const Token& next() {
    //start_of_next:
    
    // First time we need to start the input, advance to the first byte and produce a NewLine
    if (!input_->started()) {
      input_->next();
      token_.line = 1;
      token_.column = 0;
      token_.length = 0;
      token_.type = Token::NewLine;
      return token_;
    }
    
    // Skip any whitespace.
    if (IsWhitespaceByte(input_->current())) {
      uint32_t lengthAfterLF = 0;
      bool afterLF = false;
      
      do {
        
        if (IsNewLineByte(input_->current())) {
          if (!afterLF) {
            afterLF = true;
          }
          lengthAfterLF = 0;
          ++line_;
          column_ = 1;
        } else if (afterLF) {
          ++lengthAfterLF;
        }
      } while (IsWhitespaceByte(nextByte()));
      
      if (afterLF) {
        token_.line = line_;
        token_.column = 0;
        token_.length = lengthAfterLF;
        token_.type = Token::NewLine;
        return token_;
      }
    }
    
    uint32_t startColumn = column_-1;
    
    // identifier: [a-zA-Z_][a-zA-Z0-9_]*
    if (isalpha(input_->current()) || input_->current() == '_') {
      size_t datac;
      const uint8_t *datav = input_->data(datac);
      
      #define CONSUME_SYMBOL(TYPE, LEN) do {\
        token_.line = line_; \
        token_.column = startColumn; \
        token_.length = LEN; \
        token_.type = Token::TYPE; \
        nextByte(LEN); } while(0)
      #define _I8CMP(LEN, ...) (datac >= LEN && rsms_i8cmp##LEN(datav, __VA_ARGS__))
      #define if_i8CMP_then_CONSUME_SYMBOL(TYPE, LEN, ...) if (_I8CMP(LEN, __VA_ARGS__)) CONSUME_SYMBOL(TYPE, LEN)

           if_i8CMP_then_CONSUME_SYMBOL(If,           2, 'i','f');
      else if_i8CMP_then_CONSUME_SYMBOL(Else,         4, 'e','l','s','e');
      else if_i8CMP_then_CONSUME_SYMBOL(Func,         4, 'f','u','n','c');
      else if_i8CMP_then_CONSUME_SYMBOL(External,     6, 'e','x','t','e','r','n');
      else if_i8CMP_then_CONSUME_SYMBOL(None,         4, 'n','o','n','e');
      else if_i8CMP_then_CONSUME_SYMBOL(Bool,         4, 'B','o','o','l');
      else if_i8CMP_then_CONSUME_SYMBOL(IntSymbol,    3, 'I','n','t');
      else if_i8CMP_then_CONSUME_SYMBOL(FloatSymbol,  5, 'F','l','o','a','t');
      else if_i8CMP_then_CONSUME_SYMBOL(Mutable,      7, 'M','U','T','A','B','L','E');
      
      else if (_I8CMP(4, 't','r','u','e')) {
        token_.line = line_;
        token_.column = startColumn;
        token_.length = 4;
        token_.intValue = 1;
        token_.type = Token::BoolLiteral;
        nextByte(4);
      }
      else if (_I8CMP(5, 'f','a','l','s','e')) {
        token_.line = line_;
        token_.column = startColumn;
        token_.length = 5;
        token_.intValue = 0;
        token_.type = Token::BoolLiteral;
        nextByte(5);
      }
      
      else {
        token_.line = line_;
        token_.column = startColumn;
        token_.length = 1;
        token_.type = Token::Identifier;
        token_.stringValue = (char&)input_->current();
        while (isalnum(nextByte()) || input_->current() == '_')
          token_.stringValue += input_->current();
      }
      
      #undef if_i8CMP_then_CONSUME_SYMBOL
      #undef if_I8CMP
      #undef CONSUME_SYMBOL
    }
    
    /// numberliteral
    ///   :: = intliteral | floatliteral
    ///   intliteral ::= '0' | [1-9][0-9]*
    ///   floatliteral ::= [0-9] '.' [0-9]*
    else if ( isdigit(input_->current()) || input_->current() == '.') {
      token_.stringValue = input_->current();
      token_.type = input_->current() == '.' ? Token::FloatLiteral : Token::IntLiteral;
      
      while (1) {
        const uint8_t& b = nextByte();
        if (b == '.') {
          token_.type = Token::FloatLiteral;
        } else if (!isdigit(b)) {
          break;
        }
        token_.stringValue += b;
      }
      
      token_.line = line_;
      token_.column = startColumn;
      token_.length = column_ - startColumn - 1;
      
      if (token_.type == Token::IntLiteral) {
        // radix -- TODO: Support detecting 2, 8, 10, 16, and 36 radix literals
        token_.intValue = 10;
      }
    }
    
    // Comment until end of line: #.*
    else if (input_->current() == '#') {
      token_.line = line_;
      token_.column = startColumn;
      token_.type = Token::Comment;
      token_.stringValue = input_->current();
      while (nextByte() != InputEnd && input_->current() != '\n' && input_->current() != '\r') {
        token_.stringValue += input_->current();
      }
      token_.length = column_ - startColumn - 1;
      
      // This code ignores the comment instead of producing a token
      // while (nextByte() != InputEnd && input_->current() != '\n' && input_->current() != '\r');
      // if (input_->current() != InputEnd) {
      //   goto start_of_next; // return next()
      // }
    }
    
    // End
    else if (input_->ended()) {
      token_.line = line_;
      token_.column = column_;
      token_.type = Token::End;
    }
    
    // Simple 1-2 byte tokens
    else {
      // 2-byte equality operator: '!=' '<=' '>=' '=='
      if ( (input_->futureCount() > 0 && input_->future(0) == '=') && (
                input_->current() == '!'
             || input_->current() == '<'
             || input_->current() == '>'
             || input_->current() == '='
           ) )
      {
        token_.stringValue = input_->current();
        token_.stringValue += '='; //input_->future(0);
        token_.line = line_;
        token_.column = column_;
        token_.type = Token::BinaryComparisonOperator;
        //rlog("BinaryComparisonOperator: " << token_.toString());
        nextByte(); // consume
      }
      
      // Single-byte tokens
      else {

        switch (input_->current()) {
          case '<': { // '<-'?
            if (input_->futureCount() > 0 && input_->future(0) == '-') {
              token_.type = Token::LeftArrow;
              nextByte(); // consume '-'
              break;
            }
          }
          case '-': { // '->'?
            if (input_->futureCount() > 0 && input_->future(0) == '>') {
              token_.type = Token::RightArrow;
              nextByte(); // consume '>'
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
          case ',': token_.type = Token::Comma; break;
          case '.': token_.type = Token::Stop; break;

          case '{':
          case '}':
          case ']':
          case '[': token_.type = Token::Structure; break;
        
          default: {
            token_.stringValue = input_->current();
            token_.type = Token::Unexpected;
            break;
          }
        }
        
        if (token_.type != Token::Unexpected && Token::TypeInfo[token_.type].hasStringValue) {
          token_.stringValue = (char&)input_->current();
        }
        
        token_.line = line_;
        token_.column = column_;
      }
        
      nextByte(); // consume
    }
    
    return token_;
  }
  
  // void x() {
  //     while (input.hasMore()) {
  //       uint8_t &byte = input.next();
  //       cout << ">> " << byte << " (" << input.pastCount() << ", " << input.futureCount() << ")"
  //            << " \"";
  //       for (size_t x = input.pastCount(); x; --x) {
  //         cout << input.past(x-1);
  //       }
  //       cout << "\" -- \"";
  //       for (size_t x = 0; x < input.futureCount(); ++x) {
  //         cout << input.future(x);
  //       }
  //       cout << "\"" << endl;
  //     }
  //     cout << endl;
  //   }
};


} // namespace rsms

#endif // RSMS_TOKENIZER_H
