all: covec

clean:
	rm -f src/parser.cc src/tokens.cc

src/parser.cc: src/parser.y
	bison --name-prefix=cove_ -d -o $@ $^

src/parser.hh: src/parser.cc

src/tokens.cc: src/tokens.l src/parser.hh
	lex --prefix=cove_ --8bit -o $@ $^

covec: src/main.cc src/parser.cc src/tokens.cc src/codegen.cc
	llvm-g++ -o $@ `deps/llvm/bin/bin/llvm-config --libs core jit native --cxxflags --ldflags` -O1 -frtti $^

# -DNDEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3  -fno-exceptions -fno-rtti -fno-common -Woverloaded-virtual -Wcast-qual

#-fno-exceptions -fno-rtti -fno-common
#g++ -o $@ `deps/llvm/bin/bin/llvm-config --libs core jit native --cxxflags --ldflags` $^
# codegen.cc
