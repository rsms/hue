#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include "parse/FileInput.h"
#include "parse/Tokenizer.h"
#include "parse/TokenBuffer.h"
#include "parse/Parser.h"

#include "codegen/LLVMVisitor.h"

using std::cerr;
using std::cout;
using std::cin;
using std::endl;
using std::ios_base;
using std::ifstream;

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
  ast::Function *module = parser.parse();
  
  // Check for errors
  if (!module) return 1;
  if (parser.errors().size() != 0) {
    std::cerr << parser.errors().size() << " errors." << std::endl;
    return 1;
  }

  // Generate code
  codegen::LLVMVisitor codegenVisitor;
  llvm::Module *llvmModule = codegenVisitor.genModule(llvm::getGlobalContext(), "hello", module);
  
  std::cout << "moduleIR: " << llvmModule << std::endl;

  return 0;
}