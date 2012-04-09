#ifndef COVE_MODULE_H
#define COVE_MODULE_H

#include "Node.h"
#include <err.h>

typedef struct cove_source_location {
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} cove_source_location_t;

class Module {
public:
  NBlock *block;
  
  bool parseFile(FILE *inputFile); // parser.y
  
  void onUnexpectedToken(std::string text) {
    errx(42, "Unexpected token: %s", text.c_str());
  }
  
  void onParseError(cove_source_location_t *location, const char *message) {
    errx(43, "Parse error [%d:%d]-[%d:%d]: %s",
         location->first_line, location->first_column,
         location->last_line, location->last_column,
         message );
  }
};

#endif  // COVE_MODULE_H
