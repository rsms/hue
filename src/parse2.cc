#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include "parse/FileInput.h"
#include "parse/Tokenizer.h"
#include "parse/TokenBuffer.h"
#include "parse/Parser.h"

using std::cerr;
using std::cout;
using std::cin;
using std::endl;
using std::ios_base;
using std::ifstream;

using namespace rsms;

int main() {

  try {
    
    // A ByteInput that reads from a file, feeding bytes to Tokenizer
    FileInput<> input("llvm-tut/program1.txt");
    
    // A tokenizer produce tokens parsed from a ByteInput
    Tokenizer tokenizer(&input);
    
    // A TokenBuffer reads tokens from a Tokenizer and maintains limited history
    TokenBuffer tokens(tokenizer);
    
    // A parser reads the token buffer and produce an AST
    Parser parser(tokens);
    
    parser.parse();
  
  
  } catch (std::exception &e) {
    cerr << "Caught exception:" << endl;
    cerr << "  " << e.what() << endl;
  }

  return 0;
}