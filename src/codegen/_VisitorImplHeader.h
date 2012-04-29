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

namespace rsms { namespace codegen {
using namespace llvm;
