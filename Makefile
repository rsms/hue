all: covec

clean:
	rm -f covec src/parser.cc src/tokens.cc

src/parser.cc: src/parser.y
	bison --name-prefix=cove_ -d -o $@ $^

src/parser.hh: src/parser.cc

src/tokens.cc: src/tokens.l src/parser.hh
	lex --prefix=cove_ --8bit -o $@ $^

llvm_components := all
# Available components (from llvm-config --components):
#   all analysis archive asmparser asmprinter backend bitreader bitwriter
#   codegen core debuginfo engine executionengine instcombine instrumentation
#   interpreter ipa ipo jit linker mc mcdisassembler mcjit mcparser native
#   nativecodegen object runtimedyld scalaropts selectiondag support tablegen
#   target transformutils x86 x86asmparser x86asmprinter x86codegen x86desc
#   x86disassembler x86info x86utils
llvm_config := deps/llvm/bin/bin/llvm-config
llvm_inc_dir := $(shell $(llvm_config) --includedir)
llvm_lib_dir := $(shell $(llvm_config) --libdir)
llvm_flags := $(shell $(llvm_config) --libs $(llvm_components) --cxxflags --ldflags)
clang_c_flags := -I$(llvm_inc_dir)
clang_ld_flags := -L$(llvm_lib_dir) -lclang -lclangARCMigrate -lclangAST -lclangAnalysis -lclangBasic -lclangCodeGen -lclangDriver -lclangFrontend -lclangFrontendTool -lclangIndex -lclangLex -lclangParse -lclangRewrite -lclangSema -lclangSerialization -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangStaticAnalyzerFrontend
clang_flags := $(clang_c_flags) $(clang_ld_flags)

covec: src/main.cc src/parser.cc src/tokens.cc src/codegen.cc
	llvm-g++ -o $@ $(llvm_flags) $(clang_flags) -O2 $^

# -DNDEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3  -fno-exceptions -fno-rtti -fno-common -Woverloaded-virtual -Wcast-qual

#-fno-exceptions -fno-rtti -fno-common
#g++ -o $@ `deps/llvm/bin/bin/llvm-config --libs core jit native --cxxflags --ldflags` $^
# codegen.cc
