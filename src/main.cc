#include <iostream>
#include <stdio.h>
#include "Module.h"
#include "codegen.h"

int main(int argc, char **argv) {
  Module module;
  module.parseFile(stdin);
  printf("[main] module->block: %p\n", module.block);
  
  CodeGenContext context;
  context.generateCode(*module.block);
  
  context.runCode();
  
  return 0;
}
