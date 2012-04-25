// A StreamInput (: ByteInput) that reads from a file
#ifndef RSMS_FILE_INPUT_H
#define RSMS_FILE_INPUT_H

#include "StreamInput.h"

namespace rsms {

template <size_t BufferSize = 4096>
class FileInput : public StreamInput<BufferSize> {
  std::fstream stream_;
public:
  explicit FileInput(const char* filename)
      : stream_(filename, std::ifstream::in | std::ifstream::binary)
      , StreamInput<BufferSize>(stream_) {}
    
  ~FileInput() {
    stream_.close();
  }
};

} // namespace rsms

#endif // RSMS_FILE_INPUT_H
