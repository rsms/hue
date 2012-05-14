// Parses a program into an Abstract Syntax Tree (AST).
//
// clang++ -std=c++11 -o build/example-parse examples/parse.cc \
//   -Ibuild/include -Lbuild/lib -lhuert && build/example-parse
//
#include "../src/parse/FileInput.h"
#include "../src/parse/Tokenizer.h"
#include "../src/parse/TokenBuffer.h"
#include "../src/parse/Parser.h"

#include <iostream>

using namespace hue;

int main(int argc, char **argv) {
  // Read input file
  Text textSource;
  const char* filename = argc > 1 ? argv[1] : "examples/program1.hue";
  if (!textSource.setFromUTF8FileContents(filename)) {
    std::cerr << "Failed to read file '" << filename << "'" << std::endl;
    return 1;
  }
  
  // A tokenizer produce tokens parsed from a ByteInput
  Tokenizer tokenizer(textSource);
  
  // A TokenBuffer reads tokens from a Tokenizer and maintains limited history
  TokenBuffer tokens(tokenizer);
  
  // A parser reads the token buffer and produces an AST
  Parser parser(tokens);
  
  // Parse the input into an AST
  ast::Function *module = parser.parseModule();
  
  // Check for errors
  if (!module) {
    std::cerr << "Failed to parse module." << std::endl;
    return 1;
  } else if (parser.errors().size() != 0) {
    std::cerr << parser.errors().size() << " errors." << std::endl;
    return 1;
  }
  
  // Print a textual representation of the AST
  std::cout << "\e[32;1mParsed module:\e[0m " << module->toString() << std::endl;

  return 0;
}