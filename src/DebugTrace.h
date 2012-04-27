#ifndef RSMS_DEBUG_TRACE_H
#define RSMS_DEBUG_TRACE_H

#include <stdio.h>

namespace rsms {

class DebugTrace {
  const char *funcName_;
  const char *funcInterface_;
public:
  static int depth;
  DebugTrace(const char *funcName, const char *funcInterface) {
    funcName_ = funcName;
    funcInterface_ = funcInterface;
    fprintf(stderr, "%*s\e[33;1m-> %d %s  \e[30;1m%s\e[0m\n", DebugTrace::depth*2, "",
            DebugTrace::depth, funcName_, funcInterface_);
    ++DebugTrace::depth;
  }
  ~DebugTrace() {
    --DebugTrace::depth;
    fprintf(stderr, "%*s\e[33m<- %d %s  \e[30;1m%s\e[0m\n", DebugTrace::depth*2, "",
            DebugTrace::depth, funcName_, funcInterface_);
  }
};
int DebugTrace::depth = 0;

#ifndef DEBUG_TRACE
#define DEBUG_TRACE rsms::DebugTrace _DebugTrace_##__LINE__(__FUNCTION__, __PRETTY_FUNCTION__)
#endif

} // namespace rsms
#endif // RSMS_DEBUG_TRACE_H
