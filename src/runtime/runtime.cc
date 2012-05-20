// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

//#define RT_TRACE printf("RT_TRACE: %s (%s:%d)\n", __PRETTY_FUNCTION__, __FILE__, __LINE__);
#define RT_TRACE

#include "../utf8/unchecked.h"
#include "runtime.h"

#include <unistd.h>
#include <string>

#define _writef(z, fmt, ...) do { \
  char buf[z+1]; \
  int len = snprintf(buf, z+1, fmt, __VA_ARGS__); \
  write(STDOUT_FILENO, buf, len); \
} while(0)

namespace hue {

void stdout_write(const Bool v) {
  if (v) write(STDOUT_FILENO, "true", 4);
  else   write(STDOUT_FILENO, "false", 5);
}

void stdout_write(const Float v) { _writef(32, "%f", v); }
void stdout_write(const Int   v) { _writef(32, "%lld", v); }
void stdout_write(const Byte  v) { _writef(32, "%x", v); }

void stdout_write(const UChar v) {
  std::string utf8string = Text::UCharToUTF8String(v);
  write(STDOUT_FILENO, utf8string.data(), utf8string.size());
}

void stdout_write(const DataS data) {
  RT_TRACE
  write(STDOUT_FILENO, data->data, data->length);
}

void stdout_write(const TextS text) {
  RT_TRACE
  std::string utf8string;
  std::back_insert_iterator<std::string> it = std::back_inserter(utf8string);
  Int i = 0;
  for (; i != text->length; ++i) {
    utf8::unchecked::append(text->data[i], it);
  }
  write(STDOUT_FILENO, utf8string.data(), utf8string.size());
}

} // namespace hue
