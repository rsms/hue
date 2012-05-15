# Source files
cxx_sources :=  	src/main.cc \
									src/Text.cc \
									src/Logger.cc \
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

cxx_rt_sources := src/Text.cc \
                  src/Logger.cc \
                  src/runtime/runtime.cc \
                  src/runtime/Vector.cc

c_rt_sources :=

# TODO: Add ast and parse headers
rt_headers_pub := src/Text.h \
									src/Logger.h \
									src/Mangle.h \
                  src/utf8/core.h \
                  src/utf8/checked.h \
                  src/utf8/unchecked.h \
                  src/runtime/runtime.h \
                  src/runtime/object.h \
                  src/runtime/Vector.h

# Tools
CC = clang
LD = clang
CXXC = clang++

# Compiler and Linker flags for all targets
CFLAGS   += -Wall
CXXFLAGS += -std=c++11 -fno-rtti
LDFLAGS  +=
XXLDFLAGS += -lc++ -lstdc++

# Compiler and Linker flags for release targets
CFLAGS_RELEASE  := -O3 -DNDEBUG
LDFLAGS_RELEASE :=
# Compiler and Linker flags for debug targets
CFLAGS_DEBUG    := -g -O0 #-gdwarf2
LDFLAGS_DEBUG   :=

# Build dir
build_dir := build
build_lib_dir := $(build_dir)/lib
build_include_dir := $(build_dir)/include
build_bin_dir := $(build_dir)/bin
test_build_dir := ./test/build

# Source files to Object files
object_dir := $(build_dir)/.objs
cxx_objects := ${cxx_sources:.cc=.o}
_objects := $(cxx_objects)
objects = $(patsubst %,$(object_dir)/%,$(_objects))

rt_object_dir := $(build_dir)/.rt_objs
cxx_rt_objects := ${cxx_rt_sources:.cc=.o}
c_rt_objects := ${c_rt_sources:.c=.o}
_rt_objects := $(cxx_rt_objects) $(c_rt_objects)
rt_objects = $(patsubst %,$(rt_object_dir)/%,$(_rt_objects))

#compiler_object_dirs := $(foreach fn,$(objects),$(shell dirname "$(fn)"))
#rt_object_dirs := $(foreach fn,$(rt_objects),$(shell dirname "$(fn)"))
compiler_object_dirs := $(foreach fn,$(objects),$(dir $(fn)))
rt_object_dirs := $(foreach fn,$(rt_objects),$(dir $(fn)))

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
#libclang_ld_flags := -L$(libllvm_lib_dir) -lclang -lclangARCMigrate -lclangAST -lclangAnalysis -lclangBasic -lclangCodeGen -lclangDriver -lclangFrontend -lclangFrontendTool -lclangIndex -lclangLex -lclangParse -lclangRewrite -lclangSema -lclangSerialization -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangStaticAnalyzerFrontend
#
#libclang_c_flags := -I$(libllvm_inc_dir)
#libclang_cxx_flags := $(libclang_c_flags)

# --- libhuert ---------------------------------------------------------------------
rt_headers_build_dir := $(build_include_dir)/hue
rt_headers_export := $(patsubst src/%,$(rt_headers_build_dir)/%,$(rt_headers_pub))
rt_headers_export_dirs := $(foreach fn,$(rt_headers_export),$(dir $(fn)))

libhuert_c_flags := -I$(build_include_dir)
libhuert_cxx_flags := $(libhuert_c_flags)

libhuert_ld_flags := -L$(build_lib_dir) -lhuert

# --- targets ---------------------------------------------------------------------

all: release

internaldebug: CFLAGS += -fno-omit-frame-pointer
internaldebug: LDFLAGS += -faddress-sanitizer -fno-omit-frame-pointer
internaldebug: debug

debug: CFLAGS += $(CFLAGS_DEBUG)
debug: LDFLAGS += $(LDFLAGS_DEBUG)
debug: hue test

release: CFLAGS += $(CFLAGS_RELEASE)
release: LDFLAGS += $(LDFLAGS_RELEASE)
release: hue test

clean:
	rm -rf $(build_dir) $(test_build_dir)

echo_state:
	@echo objects: $(objects)
	@echo compiler_object_dirs: $(compiler_object_dirs)
	@echo rt_objects: $(rt_objects)
	@echo rt_object_dirs: $(compiler_object_dirs)
	@echo rt_headers_export: $(rt_headers_export)
	@echo rt_headers_export_dirs: $(rt_headers_export_dirs)

# ---------------------------------------------------------------------------------
# Unit tests

test: test_object
test: test_vector test_vector_perf
test: test_lang

make_test_build_dir:
	@mkdir -p $(test_build_dir)

test_mangle: libhuert make_test_build_dir $(test_build_dir)/test_mangle
	$(test_build_dir)/test_mangle

test_object: libhuert make_test_build_dir $(test_build_dir)/test_object
	$(test_build_dir)/test_object

test_vector: libhuert make_test_build_dir $(test_build_dir)/test_vector
	$(test_build_dir)/test_vector

test_vector_perf: CFLAGS += $(CFLAGS_RELEASE)
test_vector_perf: libhuert make_test_build_dir $(test_build_dir)/test_vector_perf
	$(test_build_dir)/test_vector_perf 100
	$(test_build_dir)/test_vector_perf 100000
	$(test_build_dir)/test_vector_perf 1000000
	$(test_build_dir)/test_vector_perf 10000000
#	$(test_build_dir)/test_vector_perf 100000000

#test_11: hue
#	$(build_bin_dir)/hue examples/program11-lists.txt
#	./deps/llvm/bin/bin/llvm-as -o=- out.ll | ./deps/llvm/bin/bin/llvm-ld -native $(libhuert_ld_flags) -o=out.a -
#	./out.a

# Hue language tests
test_lang: libhuert make_test_build_dir \
	         test_lang_data_literals \
				   test_lang_bools

test_lang_deps: libhuert make_test_build_dir

test_lang_data_literals: test_lang_deps $(test_build_dir)/test_lang_data_literals.hue.img
	bash -c '$(test_build_dir)/test_lang_data_literals.hue.img | grep "Hello World" >/dev/null || exit 1'

test_lang_bools: test_lang_deps $(test_build_dir)/test_lang_bools.hue.img
	bash -c '$(test_build_dir)/test_lang_bools.hue.img | grep "false" >/dev/null || exit 1'

test_lang_funcs: test_lang_deps $(test_build_dir)/test_lang_funcs.hue.img
	$(test_build_dir)/test_lang_funcs.hue.img

# test/build/X <- test/X.cc
$(test_build_dir)/%: test/%.cc
	$(CXXC) $(CFLAGS) $(CXXFLAGS) $(libhuert_cxx_flags) $(libhuert_ld_flags) -o $@ $<

# Hue LL IR bytecode to native image
# Depends on "libhuert"
# test/build/X.hue.img <- test/X.hue.ll
$(test_build_dir)/%.hue.img: $(test_build_dir)/%.hue.ll libhuert
	./deps/llvm/bin/bin/llvm-as -o=- $< | ./deps/llvm/bin/bin/llvm-ld -native $(libhuert_ld_flags) -o=$@ -

# Hue source to LL IR bytecode.
# Depends on "hue"
# test/build/X.hue.ll <- test/X.hue
$(test_build_dir)/%.hue.ll: test/%.hue hue
	$(build_bin_dir)/hue $< $@


# ---------------------------------------------------------------------------------
# Compiler

hue: libhuert hue_bin

hue_bin: $(objects)
	@mkdir -p $(build_bin_dir)
	$(LD) $(LDFLAGS) $(XXLDFLAGS) $(libllvm_ld_flags) -o $(build_bin_dir)/hue $^

# compiler C++ source -> objects
$(object_dir)/%.o: %.cc
	@mkdir -p $(compiler_object_dirs)
	$(CXXC) $(CFLAGS) $(CXXFLAGS) $(libllvm_cxx_flags) -I$(build_include_dir) -c -o $@ $<

# ---------------------------------------------------------------------------------
# Runtime

libhuert: copy_rt_headers make_build_lib_dir build_libhuert

build_libhuert: $(rt_objects)
	@mkdir -p $(build_lib_dir)
	$(LD) $(LDFLAGS) $(XXLDFLAGS) -shared -fPIC -o $(build_lib_dir)/libhuert.dylib $^
	$(shell ln -sf "libhuert.dylib" "$(build_lib_dir)/libhuert.so")

make_build_lib_dir:
	@mkdir -p $(build_lib_dir)

# Headers
copy_rt_headers: make_rt_headers_dirs $(rt_headers_export)

make_rt_headers_dirs:
	@mkdir -p $(rt_headers_export_dirs)

$(rt_headers_build_dir)/%.h: src/%.h
	@cp $^ $@

# runtime C source -> objects
$(rt_object_dir)/%.o: %.c
	@mkdir -p $(rt_object_dirs)
	$(CC) $(CFLAGS) -fPIC -I$(build_include_dir) -c -o $@ $<

# runtime C++ source -> objects
$(rt_object_dir)/%.o: %.cc
	@mkdir -p $(rt_object_dirs)
	$(CXXC) $(CFLAGS) $(CXXFLAGS) -fPIC -I$(build_include_dir) -c -o $@ $<



.PHONY: all hue libhuert copy_rt_headers test test_lang test_lang_deps
