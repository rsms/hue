// A StreamInput (: ByteInput) that reads from a file
#ifndef HUE__FILE_INPUT_H
#define HUE__FILE_INPUT_H

#include "StreamInput.h"
#include <fstream>

namespace hue {

template <size_t BufferSize = 4096>
class FileInput : public StreamInput<BufferSize> {
public:
  explicit FileInput(const char* filename) : StreamInput<BufferSize>() {
    StreamInput<BufferSize>::setInputStream(
      new std::ifstream(filename, std::ifstream::in | std::ifstream::binary), true);
  }
    
  virtual ~FileInput() {
    if (((std::ifstream*)this->inputStream())->is_open()) {
      ((std::ifstream*)this->inputStream())->close();
    }
  }
};

} // namespace hue

#endif // HUE__FILE_INPUT_H
