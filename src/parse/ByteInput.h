// Baseclass for byte inputs, providing limited random access to a sequence of bytes
#ifndef HUE__BYTE_INPUT_H
#define HUE__BYTE_INPUT_H

#include <stdint.h>
#include <stdlib.h>

namespace hue {

class ByteInput {
public:
  virtual bool started() const = 0;
  virtual bool ended() const = 0;
  virtual bool failed() const = 0;
  virtual const uint8_t& next(const size_t stride = 1) = 0;
  virtual const uint8_t& past(size_t offset) const = 0;
  virtual const uint8_t& current() const = 0;
  virtual const uint8_t& future(size_t offset) const = 0;
  virtual size_t pastCount() const = 0;
  virtual size_t futureCount() const = 0;
  virtual const uint8_t *data(size_t &size) const = 0;
};

static const uint8_t InputEnd = 0;

} // namespace hue

#endif // HUE__BYTE_INPUT_H
