#include "../Logger.h"
#include "Visitor.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <algorithm>
#include <iostream>
#include <fstream>

#include <llvm/ValueSymbolTable.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/ADT/ArrayRef.h>

namespace hue { namespace codegen {
using namespace llvm;


// String formatting helper.
//  std::string R_FMT(any << ..)
//
static std::ostringstream* __attribute__((unused)) _r_fmt_begin() { return new std::ostringstream; }
static std::string __attribute__((unused)) _r_fmt_end(std::ostringstream* s) {
  std::string str = s->str();
  delete s;
  return str;
}
#define R_FMT(A) _r_fmt_end( static_cast<std::ostringstream*>(&((*_r_fmt_begin()) << A )) )
