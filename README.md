# Hue

A functional programming language.

I'm just having some fun. This is by no means the futurez of programming, brogramming or anything like that (except it will be renamed to skynet in 2018 and take over the world, but more on that later).

- Everything is an expression
- Values are immutable (can't be modified) by default
- Compiles down to super-tight machine code (thanks to LLVM) that's so fast it makes your mama faint
- Built on top of LLVM and thus generates LLVM IR
- Neat codebase with clearly separated components
  - Tokenizer: Reads UTF-8 encoded text and streams Hue language tokens
  - Parser: Reads Hue language tokens and streams Hue language structures
  - Compiler ("codegen"): Reads Hue language structures and streams LLVM structures
  - Runtime library: Provides a few select features like stdio access
- Contains a immutable persistent vector implementation inspired by Clojure that's pretty darn fast. Technically O(log32(n)) random access complexity, which is almost constant, which is awesome.

Objectives and plans:

1. Have fun
2. Take is slowly and go bottom-up, analyzing machine code and drinking coffee
3. Functions that can capture its environment
4. Complex types (records/structs/et al) with automatic reference counting
5. Listen to Black Sabbath and take over the worlds

Seriously, this is just for fun. Don't expect anything from this project.

See LICENSE for the standard MIT license.
