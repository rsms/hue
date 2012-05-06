#ifndef RSMS_COVE_TEXT_H
#define RSMS_COVE_TEXT_H
#include <string>
#include <istream>
#include <stdint.h>

namespace rsms {

typedef uint32_t UChar;

class Text : public std::basic_string<UChar> {
public:
  bool setFromUTF8InputStream(std::istream& ifs, size_t length = 0);
  bool setFromUTF8FileContents(const char* filename);
  bool setFromUTF8String(const std::string& data);
  inline bool setFromUTF8String(const char* cstr) { return setFromUTF8String(std::string(cstr)); }
  bool setFromUTF8Data(const uint8_t* data, const size_t length);
  std::string UTF8String() const;
  inline std::string toString() const { return UTF8String(); }
  
  inline Text& operator= (const char* rhs) {
    setFromUTF8String(rhs);
    return *this;
  }

  inline Text& operator= (const std::string& rhs) {
    setFromUTF8String(rhs);
    return *this;
  }

  inline Text& operator= (const UChar& rhs) {
    assign(1, rhs);
    return *this;
  }
  
  // SP | TAB | CR | LF
  inline static bool isWhitespace(const UChar& c) {
    return c == 0x20 || c == 0x09 || c == 0x0d || c == 0x0a;
  }
  
  // LF
  inline static bool isLineSeparator(const UChar& c) {
    return c == 0x0a;
  }
  
  // 0-9
  inline static bool isDecimalDigit(const UChar& c) {
    return (c < 0x3a && c > 0x2f) // ascii 0-9
        || (c < 0xff1a && c > 0xff0f) // fullwidth 0-9
        ;
  }
  
  // 0-9 | A-F
  inline static bool isUpperHexDigit(const UChar& c) {
    return isDecimalDigit(c)
        || (c < 0x47 && c > 0x40) // A-F
        || (c < 0xff27 && c > 0xff20) // fullwidth A-F
        ;
  }
  
  // 0-9 | a-f
  inline static bool isLowerHexDigit(const UChar& c) {
    return isDecimalDigit(c)
        || (c < 0x67 && c > 0x60) // a-f
        || (c < 0xff47 && c > 0xff40) // fullwidth a-f
        ;
  }
  
  // 0-9 | A-F | a-f
  inline static bool isHexDigit(const UChar& c) {
    return isUpperHexDigit(c)
        || (c < 0x67 && c > 0x60) // a-f
        || (c < 0xff47 && c > 0xff40) // fullwidth a-f
        ;
  }
  
  inline static bool isPrintableASCII(const UChar& c) {
    return c < 0x7f && c > 0x20;
  }
};

extern const UChar NullChar;

} // namespace rsms

// std::ostream operator so we can write Text objects to an ostream (as UTF-8)
inline static std::ostream& operator<< (std::ostream& os, const rsms::Text& text) {
  return os << text.toString();
}

// inline static Text& operator+= (Text& lhs, const std::string& rhs) {
//   lhs.reset();
//   lhs
//   return lhs;
// }

#endif // RSMS_COVE_TEXT_H
