// A ByteInput implementation that reads from a standard input stream
#ifndef RSMS_STREAM_INPUT_H
#define RSMS_STREAM_INPUT_H

#include "ByteInput.h"
#include <iostream>
#include <fstream>

namespace rsms {

template <size_t BufferSize = 4096>
class StreamInput : public ByteInput {
  size_t  start_;  // index of oldest element
  size_t  count_;  // number of used elements
  size_t  next_;   // current element index (offset from start_)
  size_t  current_;
  uint8_t elems_[BufferSize]; // vector of elements
  std::istream &istream_;
public:
  explicit StreamInput(std::istream &ins)
    : istream_(ins)
    , start_(0)
    , count_(0)
    , next_(0)
    , current_(0) {}

  const size_t size() const { return BufferSize; }
  const size_t &count() const { return count_; }

  bool started() const { return count_ != 0; }
  bool isFull() const { return count_ == BufferSize; }
  int isEmpty() const { return count_ == 0; }

  void push(const uint8_t &elem) {
    size_t end = (start_ + count_) % BufferSize;
    elems_[end] = elem;
    if (count_ == BufferSize) {
      start_ = (start_ + 1) % BufferSize; // full, overwrite
      -- next_;
    } else {
      ++ count_;
    }
  }

  void pop(uint8_t &elem) {
    elem = elems_[start_];
    start_ = (start_ + 1) % BufferSize;
    -- next_;
    -- count_;
  }

  void pushFromStream(size_t limit) {
    if (limit > BufferSize) {
      limit = BufferSize;
    }
    //std::cout << "read " << limit << " \"";
    while (istream_.good() && limit--) {
      char byte;
      istream_.get(byte);
      if (istream_.eof()) {
        //std::cout << "<EOF>";
        break;
      }
      //std::cout << byte;
      push((const uint8_t &)byte);
    }
    //std::cout << "\"" << std::endl;
  }

  bool ended() const {
    return current_ == count_;
  }

  const uint8_t& next(const size_t stride = 1) {
    size_t lowWatermark = BufferSize / 3;
    size_t futurec = futureCount();
  
    if (futurec < lowWatermark && !istream_.eof()) {
      //std::cout << "Filling from stream... " << std::endl;
      size_t readLimit;
      if (!isFull() && futurec == 0) {
        readLimit = BufferSize; // first read
      } else {
        readLimit = BufferSize / 2;
      }
      pushFromStream(readLimit);
      //cout << "count_: " << count_ << endl;
    } else if (futurec == 0 && istream_.eof()) {
      current_ = count_;
      return InputEnd;
    }

    current_ = (start_ + next_ + (stride - 1)) % BufferSize;
    next_ += stride;
    return elems_[current_];
  }

  const uint8_t& past(size_t offset) const {
    size_t cur = (start_ + next_ - offset - 2) % BufferSize;
    return elems_[cur];
  }
  
  const uint8_t& current() const {
    if (current_ == count_) {
      return InputEnd;
    } else {
      return elems_[current_];
    }
  }

  const uint8_t& future(size_t offset) const {
    size_t cur = (start_ + next_ + offset) % BufferSize;
    return elems_[cur];
  }

  size_t pastCount() const {
    return count_ - (count_ - next_ + 1);
  }

  size_t futureCount() const {
    return count_ - next_;
  }
  
  const uint8_t *data(size_t &size) const {
    size = count_ - current_;
    return elems_ + current_;
  }
};

} // namespace rsms

#endif // RSMS_STREAM_INPUT_H
