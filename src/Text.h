#ifndef RSMS_COVE_TEXT_H
#define RSMS_COVE_TEXT_H
#include <string>
#include <istream>
#include <stdint.h>

namespace rsms {

// Represents a Unicode character (or "codepoint" if you will)
typedef uint32_t UChar;

// Represents a sequence of bytes.
typedef std::basic_string<uint8_t> ByteString;

// Represents Unicode text.
class Text : public std::basic_string<UChar> {
public:
  // Replace the contents of the receiver by reading the istream, which is expected to
  // produce UTF-8 data.
  // Returns false if the stream could not be read or the contents is not UTF-8 text.
  bool setFromUTF8InputStream(std::istream& ifs, size_t length = 0);
  
  // Replace the contents of the receiver with the contents of the file pointed to by
  // *filename*. The contents of the file is expected to be encoded as UTF-8.
  // Returns false if the file could not be read or the contents is not UTF-8 text.
  bool setFromUTF8FileContents(const char* filename);
  
  // Replace the contents of the receiver by decoding UTF-8 data.
  bool setFromUTF8String(const std::string& data);
  inline bool setFromUTF8String(const char* cstr) { return setFromUTF8String(std::string(cstr)); }
  bool setFromUTF8Data(const uint8_t* data, const size_t length);
  
  // Create a UTF-8 representation of the text. Returns an empty string if encoding failed.
  std::string UTF8String() const;
  inline std::string toString() const { return UTF8String(); } // alias
  
  // Create a sequence of bytes by interpreting each character in the text as 1-4 bytes.
  // E.g. A 3 octet wide unicode character U+1F44D is represented as the bytes 0x1, 0xF4 and 0x4D.
  ByteString rawByteString() const;
  
  // Assignment operators
  inline Text& operator= (const char* rhs) {
    setFromUTF8String(rhs); return *this;
  }
  inline Text& operator= (const std::string& rhs) {
    setFromUTF8String(rhs); return *this;
  }
  inline Text& operator= (const UChar& rhs) {
    assign(1, rhs); return *this;
  }
  
  // Convert Unicode character c to its UTF8 equivalent.
  // Returns an empty string on failure.
  static std::string UCharToUTF8String(const UChar c);
  
  // LF | CR
  inline static bool isLineSeparator(const UChar& c) {
    return c == 0x0a || c == 0x0d;
  }
  
  // SP | TAB
  inline static bool isWhitespace(const UChar& c) {
    return c == 0x20 || c == 0x09;
  }
  
  // SP | TAB | LF | CR
  inline static bool isWhitespaceOrLineSeparator(const UChar& c) {
    return isWhitespace(c) || isLineSeparator(c);
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
  
  inline static bool isPrintable(const UChar& c) {
    // TODO: Actual list of printable chars in Unicode 6.1 (ZOMG that's a long list of tests)
    return isPrintableASCII(c) || c > 0x7f;
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
