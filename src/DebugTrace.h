// Copyright (c) 2012, Rasmus Andersson. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.
#ifndef HUE__DEBUG_TRACE_H
#define HUE__DEBUG_TRACE_H

#include <stdio.h>

static volatile int g_DebugTrace_depth = 0;

namespace hue {

class DebugTrace {
  const char *primary_;
  const char *secondary_;
  int line_;
public:
  DebugTrace(const char *primary, const char *secondary, int line = 0)
      : primary_(primary), secondary_(secondary), line_(line) {
    fprintf(stderr, "%*s\e[33;1m-> %d %s  \e[30;1m%s:%d\e[0m\n", g_DebugTrace_depth*2, "",
            g_DebugTrace_depth, primary_, secondary_, line_);
    ++g_DebugTrace_depth;
  }
  ~DebugTrace() {
    --g_DebugTrace_depth;
    fprintf(stderr, "%*s\e[33m<- %d %s  \e[30;1m%s:%d\e[0m\n", g_DebugTrace_depth*2, "",
            g_DebugTrace_depth, primary_, secondary_, line_);
  }
};


#ifndef DEBUG_TRACE
#define DEBUG_TRACE hue::DebugTrace _DebugTrace_##__LINE__(__FUNCTION__, __FILE__, __LINE__)
#endif

} // namespace hue
#endif // HUE__DEBUG_TRACE_H
