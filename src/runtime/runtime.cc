#define RT_TRACE printf("RT_TRACE: %s (%s:%d)\n", __PRETTY_FUNCTION__, __FILE__, __LINE__)

#include "../utf8/unchecked.h"
#include "runtime.h"
#include <unistd.h>
#include <string>


void cove_stdout_write(const COByteArray data) {
  RT_TRACE;
  write(0, data->data, data->length);
}

void cove_stdout_write(const COText text) {
  RT_TRACE;
  std::string utf8string;
  std::back_insert_iterator<std::string> it = std::back_inserter(utf8string);
  COInt i = 0;
  for (; i != text->length; ++i) {
    utf8::unchecked::append(text->data[i], it);
  }
  write(0, utf8string.data(), utf8string.size());
}

