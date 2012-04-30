#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include "codegen/Visitor.h"

#include "parse/FileInput.h"
#include "parse/Tokenizer.h"
#include "parse/TokenBuffer.h"
#include "parse/Parser.h"

#include <llvm/Support/raw_ostream.h>
#include <fcntl.h>

using namespace rsms;

int main(int argc, char **argv) {
  
  // A FileInput reads from a file, feeding bytes to a Tokenizer
  FileInput<> input(argc > 1 ? argv[1] : "examples/program1.txt");
  if (input.failed()) {
    std::cerr << "Failed to open input." << std::endl;
    return 1;
  }
  
  // A tokenizer produce tokens parsed from a ByteInput
  Tokenizer tokenizer(&input);
  
  // A TokenBuffer reads tokens from a Tokenizer and maintains limited history
  TokenBuffer tokens(tokenizer);
  
  // A parser reads the token buffer and produce an AST
  Parser parser(tokens);
  
  // Parse the input into an AST
  ast::Function *moduleFunc = parser.parseModule();
  if (!moduleFunc) return 1;
  if (parser.errors().size() != 0) {
    std::cerr << parser.errors().size() << " parse error(s)." << std::endl;
    return 1;
  }
  std::cout << "Parsed module: " << moduleFunc->body()->toString() << std::endl;
  return 0; // xxx only parser
  
  // Generate code
  codegen::Visitor codegenVisitor;
  llvm::Module *llvmModule = codegenVisitor.genModule(llvm::getGlobalContext(), "hello", moduleFunc);
  //std::cout << "moduleIR: " << llvmModule << std::endl;
  if (!llvmModule) {
    std::cerr << codegenVisitor.errors().size() << " error(s) during code generation." << std::endl;
    return 1;
  }
  
  // Write IR to stderr
  llvmModule->dump();
  
  // Write human-readable IR to file "out.ll"
  std::string errInfo;
  llvm::raw_fd_ostream os("out.ll", errInfo);
  if (os.has_error()) {
    std::cerr << "Failed to open 'out.ll' file for output. " << errInfo << std::endl;
    return 1;
  }
  llvmModule->print(os, 0);
  
  //
  // From here:
  //
  //   PATH=$PATH:$(pwd)/deps/llvm/bin/bin
  //
  //   Run the generated program:
  //     lli out.ll
  //
  //   Generate target assembler:
  //     llc -asm-verbose -o=- out.ll
  //
  //   Generate target image:
  //     llvm-as -o=- out.ll | llvm-ld -native -o=out.a -
  //
  // See http://llvm.org/docs/CommandGuide/ for details on these tools.
  //
  
  return 0;
}