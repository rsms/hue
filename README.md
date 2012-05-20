# Hue

A functional programming language.

I'm just having some fun. This is by no means the futurez of programming, brogramming or anything like that (except it will be renamed to skynet in 2018 and take over the world, but more on that later).

- Everything is an expression
- Values are immutable (can't be modified) by default
- Compiles down to super-tight machine code (thanks to LLVM) that's so fast it makes your mama faint
- Fully Unicode (Text type is UTF-32, source files are interpreted as UTF-8 and language symbols can be almost any Unicode character).
- Built on top of LLVM and thus can generate LLVM IR
- Fast JIT compiler
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

## Building

First, you need to grab and build llvm. See `deps/llvm/README` for details.

Then, it's all just regular make:

    $ make

Should give you some stuff in the `build` subdirectory.

## bin/hue

The `hue` program is a all-in-one tool which is essentially a JIT dynamic compiler.

    $ make hue
    $ build/bin/hue --help
    $ build/bin/hue test/test_lang_data_literals.hue
    Hello World
    Hello World
    $ build/bin/hue -output-ir=- -compile-only test/test_lang_data_literals.hue
    # IR code here...
    $ build/bin/hue -parse-only test/test_lang_data_literals.hue
    # AST repr here...

You could chain hue with llvm tools to compile a machine image:

    $ build/bin/hue -output-ir=- -compile-only test/test_lang_data_literals.hue \
      | llvm-as -o=- | llvm-ld -native -Lbuild/lib -lhuert -o=program -
    $ ./program
    Hello World
    Hello World

If you don't have a local llvm installation, you might need to add the llvm bin directory from deps to your PATH environment variable before running the above.

    PATH=$PATH:$(pwd)/deps/llvm/bin/bin

## License

See LICENSE for the standard MIT license.
