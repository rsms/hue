# Source files
cxx_sources := src/main.cc src/Logger.cc src/Text.cc \
               src/codegen/Visitor.cc \
               src/codegen/assignment.cc \
               src/codegen/binop.cc \
               src/codegen/call.cc \
               src/codegen/function.cc \
               src/codegen/function_type.cc \
               src/codegen/cast.cc \
               src/codegen/conditional.cc
               

# Tools
CC = clang
LD = clang
CXXC = clang++

# Compiler and Linker flags for all targets
CFLAGS   += -Wall
CXXFLAGS += -std=c++11
LDFLAGS  += -lc++ -lstdc++

# Compiler and Linker flags for release targets
CFLAGS_RELEASE  := -O3 -DNDEBUG
LDFLAGS_RELEASE :=
# Compiler and Linker flags for debug targets
CFLAGS_DEBUG    := -g -O0 #-gdwarf2
LDFLAGS_DEBUG   :=

# Source files to Object files
object_dir := .objs

cxx_objects := ${cxx_sources:.cc=.o}
_objects := $(cxx_objects)
objects = $(patsubst %,$(object_dir)/%,$(_objects))
object_dirs := $(foreach fn,$(objects),$(shell dirname "$(fn)"))

# --- libllvm ---------------------------------------------------------------------
libllvm_components := all
# Available components (from llvm-config --components):
#   all analysis archive asmparser asmprinter backend bitreader bitwriter
#   codegen core debuginfo engine executionengine instcombine instrumentation
#   interpreter ipa ipo jit linker mc mcdisassembler mcjit mcparser native
#   nativecodegen object runtimedyld scalaropts selectiondag support tablegen
#   target transformutils x86 x86asmparser x86asmprinter x86codegen x86desc
#   x86disassembler x86info x86utils
llvm_config := deps/llvm/bin/bin/llvm-config
libllvm_inc_dir := $(shell $(llvm_config) --includedir)
libllvm_lib_dir := $(shell $(llvm_config) --libdir)
libllvm_ld_flags := $(shell $(llvm_config) --libs $(libllvm_components) --ldflags)

libllvm_c_flags := -I$(libllvm_inc_dir)
libllvm_c_flags += -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS
libllvm_cxx_flags := $(libllvm_c_flags) -fno-rtti -fno-common

# --- libclang ---------------------------------------------------------------------
libclang_ld_flags := -L$(libllvm_lib_dir) -lclang -lclangARCMigrate -lclangAST -lclangAnalysis -lclangBasic -lclangCodeGen -lclangDriver -lclangFrontend -lclangFrontendTool -lclangIndex -lclangLex -lclangParse -lclangRewrite -lclangSema -lclangSerialization -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangStaticAnalyzerFrontend

libclang_c_flags := -I$(libllvm_inc_dir)
libclang_cxx_flags := $(libclang_c_flags)

# --- targets ---------------------------------------------------------------------

#all: echo_state covec
all: release

internaldebug: CFLAGS += -fno-omit-frame-pointer
internaldebug: LDFLAGS += -faddress-sanitizer -fno-omit-frame-pointer
internaldebug: debug

debug: CFLAGS += $(CFLAGS_DEBUG)
debug: LDFLAGS += $(LDFLAGS_DEBUG)
debug: covec

release: CFLAGS += $(CFLAGS_RELEASE)
release: LDFLAGS += $(LDFLAGS_RELEASE)
release: covec

clean:
	rm -f covec
	rm -rf $(object_dir)

echo_state:
	@echo objects: $(objects)
	@echo object_dirs: $(object_dirs)

# C++ source -> objects
$(object_dir)/%.o: %.cc
	@mkdir -p $(object_dirs)
	$(CXXC) $(CFLAGS) $(CXXFLAGS) $(libllvm_cxx_flags) -c -o $@ $<

covec: $(objects)
	$(LD) $(LDFLAGS) $(libllvm_ld_flags) -o covec $^

.PHONY: 