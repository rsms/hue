#ifndef RSMS_DEBUG_TRACE_H
#define RSMS_DEBUG_TRACE_H

#include <stdio.h>

static volatile int g_DebugTrace_depth = 0;

namespace rsms {

class DebugTrace {
  const char *funcName_;
  const char *funcInterface_;
public:
  DebugTrace(const char *funcName, const char *funcInterface) {
    funcName_ = funcName;
    funcInterface_ = funcInterface;
    fprintf(stderr, "%*s\e[33;1m-> %d %s  \e[30;1m%s\e[0m\n", g_DebugTrace_depth*2, "",
            g_DebugTrace_depth, funcName_, funcInterface_);
    ++g_DebugTrace_depth;
  }
  ~DebugTrace() {
    --g_DebugTrace_depth;
    fprintf(stderr, "%*s\e[33m<- %d %s  \e[30;1m%s\e[0m\n", g_DebugTrace_depth*2, "",
            g_DebugTrace_depth, funcName_, funcInterface_);
  }
};


#ifndef DEBUG_TRACE
#define DEBUG_TRACE rsms::DebugTrace _DebugTrace_##__LINE__(__FUNCTION__, __PRETTY_FUNCTION__)
#endif

} // namespace rsms
#endif // RSMS_DEBUG_TRACE_H
