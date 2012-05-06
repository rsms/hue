#include "Text.h"
#include "utf8/checked.h"

#include <fstream>
//#include <iostream>

namespace rsms {


bool Text::setFromUTF8String(const std::string& utf8string) {
  clear();
  bool ok = true;
  try {
    utf8::utf8to32(utf8string.begin(), utf8string.end(), std::back_inserter(*this));
  } catch (const std::exception &e) {
    clear();
    ok = false;
  }
  return ok;
}


bool Text::setFromUTF8Data(const uint8_t* data, const size_t length) {
  return setFromUTF8String(std::string(reinterpret_cast<const char*>(data), length));
}


bool Text::setFromUTF8InputStream(std::istream& is, size_t length) {
  if (!is.good()) return false;
  
  char *buf = NULL;
  std::string utf8string;
  
  if (length != 0) {
    buf = new char[length];
    is.read(buf, length);
    utf8string.assign(buf, length);
  } else {
    length = 4096;
    buf = new char[length];
    while (is.good()) {
      is.read(buf, length);
      utf8string.append(buf, is.gcount());
    }
  }
  
  return setFromUTF8String(utf8string);
}


bool Text::setFromUTF8FileContents(const char* filename) {
  std::ifstream ifs(filename, std::ifstream::in | std::ifstream::binary);
  if (!ifs.good()) return false;
  
  ifs.seekg(0, std::ios::end);
  size_t length = ifs.tellg();
  ifs.seekg(0, std::ios::beg);

  bool ok = setFromUTF8InputStream(ifs, length);
  ifs.close();
  return ok;
}


std::string Text::UTF8String() const {
  std::string utf8string;
  try {
    utf8::utf32to8(this->begin(), this->end(), std::back_inserter(utf8string));
  } catch (const std::exception &e) {
    utf8string.clear();
  }
  return utf8string;
}


const UChar NullChar = 0;

} // namespace rsms
