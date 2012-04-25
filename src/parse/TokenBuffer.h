// A buffer that reads tokens from a Tokenizer and keeps N historical
// tokens around.
#ifndef RSMS_TOKEN_BUFFER_H
#define RSMS_TOKEN_BUFFER_H

#include "Token.h"
#include "Tokenizer.h"

namespace rsms {

#define TokenBufferSize 16

class TokenBuffer {
  Tokenizer &tokenizer_;
  size_t  start_;  // index of oldest element
  size_t  count_;  // number of used elements
  size_t  next_;   // index of oldest element
  Token   tokens_[TokenBufferSize]; // vector of elements
  
public:
  explicit TokenBuffer(Tokenizer &tokenizer) : tokenizer_(tokenizer), start_(0), count_(0), next_(0) {}

  const size_t size() const { return TokenBufferSize; }
  const size_t& count() const { return count_; }
  
  void push(const Token& token) {
    size_t end = (start_ + count_) % TokenBufferSize;
    tokens_[end] = token;
    if (count_ == TokenBufferSize) {
      start_ = (start_ + 1) % TokenBufferSize; // full, overwrite
    } else {
      ++count_;
    }
  }

  // const Token& pop() {
  //   size_t start = start_;
  //   start_ = (start_ + 1) % TokenBufferSize;
  //   --count_;
  //   return tokens_[start];
  // }
  
  const Token& next() {
    const Token& token = tokenizer_.next();
    push(token);
    ++next_;
    return token;
  }
  
  // History access. 0=last token returned by next(), N=N tokens back in time
  const Token& operator[](size_t index) const {
    size_t i;
    if (count_ == TokenBufferSize) {
      i = (start_ + (count_ - (index + 1))) % TokenBufferSize;
    } else {
      i = count_ - (index + 1);
    }
    return tokens_[i];
  }
};


} // namespace rsms

#endif // RSMS_TOKEN_BUFFER_H
