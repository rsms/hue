// Tokenizes a program -- essentially reading lexical components.
// clang++ -std=c++11 -o tokenize examples/tokenize.cc src/Text.cc src/Logger.cc && ./tokenize
#include "../src/parse/Tokenizer.h"
#include "../src/parse/TokenBuffer.h"
#include "../src/parse/FileInput.h"

#include <iostream>

using namespace rsms;

int main(int argc, char **argv) {
  // Read input file
  Text textSource;
  const char* filename = argc > 1 ? argv[1] : "examples/program1.txt";
  if (!textSource.setFromUTF8FileContents(filename)) {
    std::cerr << "Failed to read file '" << filename << "'" << std::endl;
    return 1;
  }
  
  // A tokenizer produce tokens parsed from a ByteInput
  Tokenizer tokenizer(textSource);
  
  // A TokenBuffer reads tokens from a Tokenizer and maintains limited history
  TokenBuffer tokens(tokenizer);

  while (1) {
    const Token &token = tokens.next();
    
    if (token.type == Token::Unexpected) {
      std::cout << "Error: Unexpected token '" << token.textValue
                << "' at " << token.line << ':' << token.column << std::endl;
      break;
    }
    
    std::cout << "\e[34;1m>> " << token.toString() << "\e[0m";
    
    // list all available historical tokens
    // size_t n = 1;
    // while (n < tokens.count()) {
    //   std::cout << "\n  " << tokens[n++].toString() << ")";
    // }
    std::cout << std::endl;
    
    // An End token indicates the end of the token stream and we must
    // stop reading the stream.
    if (token.type == Token::End || token.type == Token::Error) break;
  }

  return 0;
}