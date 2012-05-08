# Source files
cxx_sources :=  	src/main.cc src/Logger.cc src/Text.cc \
                	src/codegen/Visitor.cc \
                	src/codegen/assignment.cc \
                	src/codegen/binop.cc \
                	src/codegen/call.cc \
                	src/codegen/function.cc \
                	src/codegen/function_type.cc \
                	src/codegen/cast.cc \
                	src/codegen/conditional.cc \
                	src/codegen/data_literal.cc \
                	src/codegen/text_literal.cc

cxx_rt_sources := src/runtime/runtime.cc src/Text.cc
#cxx_rt_sources :=
c_rt_sources :=

# Tools
CC = clang
LD = clang
CXXC = clang++

# Compiler and Linker flags for all targets
CFLAGS   += -Wall
CXXFLAGS += -std=c++11
LDFLAGS  +=
XXLDFLAGS += -lc++ -lstdc++

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

rt_object_dir := .rt_objs
cxx_rt_objects := ${cxx_rt_sources:.cc=.o}
c_rt_objects := ${c_rt_sources:.c=.o}
_rt_objects := $(cxx_rt_objects) $(c_rt_objects)
rt_objects = $(patsubst %,$(rt_object_dir)/%,$(_rt_objects))

compiler_object_dirs := $(foreach fn,$(objects),$(shell dirname "$(fn)"))
rt_object_dirs := $(foreach fn,$(rt_objects),$(shell dirname "$(fn)"))

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
debug: covec libcovert

release: CFLAGS += $(CFLAGS_RELEASE)
release: LDFLAGS += $(LDFLAGS_RELEASE)
release: covec libcovert

clean:
	rm -f covec
	rm -rf $(object_dir) $(rt_object_dir)

echo_state:
	@echo objects: $(objects)
	@echo compiler_object_dirs: $(compiler_object_dirs)
	@echo rt_objects: $(rt_objects)
	@echo rt_object_dirs: $(compiler_object_dirs)

test_10: libcovert covec
	./covec examples/program10-data-literals.txt
	./deps/llvm/bin/bin/llvm-as -o=- out.ll | ./deps/llvm/bin/bin/llvm-ld -native -L. -lcovert -o=out.a -
	./out.a

# compiler C++ source -> objects
$(object_dir)/%.o: %.cc
	@mkdir -p $(compiler_object_dirs)
	$(CXXC) $(CFLAGS) $(CXXFLAGS) $(libllvm_cxx_flags) -c -o $@ $<

# runtime C source -> objects
$(rt_object_dir)/%.o: %.c
	@mkdir -p $(rt_object_dirs)
	$(CC) $(CFLAGS) -c -o $@ $<

# runtime C++ source -> objects
$(rt_object_dir)/%.o: %.cc
	@mkdir -p $(rt_object_dirs)
	$(CXXC) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

covec: $(objects)
	$(LD) $(LDFLAGS) $(XXLDFLAGS) $(libllvm_ld_flags) -o covec $^

libcovert: $(rt_objects)
	$(LD) $(LDFLAGS) $(XXLDFLAGS) -shared -o libcovert.so $^
	$(shell ln -sf libcovert.so libcovert.dylib)

.PHONY: 