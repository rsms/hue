#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include "../src/parse/Tokenizer.h"
#include "../src/parse/TokenBuffer.h"
#include "../src/parse/FileInput.h"

using std::cerr;
using std::cout;
using std::cin;
using std::endl;
using std::ios_base;
using std::ifstream;

using namespace rsms;

int main() {

  // A ByteInput that reads from a file, feeding bytes to Tokenizer
  FileInput<> input("llvm-tut/program1.txt");
  
  // A tokenizer produce tokens parsed from a ByteInput
  Tokenizer tokenizer(&input);
  
  // A TokenBuffer reads tokens from a Tokenizer and maintains limited history
  TokenBuffer<> tokens(tokenizer);

  while (1) {
    const Token &token = tokens.next();
    if (token.type == Token::End) {
      break;
    } else if (token.type == Token::Unexpected) {
      cout << "Error: Unexpected token '" << token.stringValue
           << "' at " << token.line << ':' << token.column << endl;
      break;
    }
    cout << "got token: " << token.toString();
    
    // list all available historical tokens
    size_t n = 1;
    while (n < tokens.count()) {
      cout << "\n  " << tokens[n++].toString() << ")";
    }
    
    cout << endl;
  }

  return 0;
}