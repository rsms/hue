# Hue

A functional programming language.

I'm just having some fun. This is by no means the futurez of programming, brogramming or anything like that (except it will be renamed to skynet in 2018 and take over the world, but more on that later).

- Everything is an expression
- Values are immutable (can't be modified)
- Fully Unicode (Text type is UTF-32, source files are interpreted as UTF-8 and language symbols can be almost any Unicode character).
- Fast JIT compiler
- Close to the metal — built on top of LLVM and thus compiles down to highly optimized machine code that's so fast it makes your mama faint
- Neat codebase with clearly separated components
  - Tokenizer: Reads UTF-8 encoded text and streams Hue language tokens.
  - Parser: Reads Hue language tokens and streams Hue language structures.
  - Transformer: Reads Reads Hue language structures (AST) and transforms the code (i.e. finalizes incomplete function types and infers expression result values).
  - Compiler ("codegen"): Reads Hue language structures (AST) and streams LLVM structures.
  - Runtime library: Provides a few select features like stdio access
- Features an immutable persistent vector implementation inspired by Clojure that's pretty darn fast (almost constant time complexity.)

## Examples

A classic example of a recursive procedure is the function used to calculate the factorial of a natural number. When compiled with optimizations in Hue, this becomes a true tail recursive function.

    factorial = func (n Int) if n == 0 1 else n * factorial n - 1
    factorial 10  # -> 3628800

The well known mathematical recursive function that computes the Nth Fibonacci number:

    fib = func (n Int)
      if n < 2
        n
      else
        (fib n-1) + fib n-2

    fib 32  # -> 2178309

Have a look at the tests in the `test` directory for more examples.

## Building

First, you need to grab and build llvm. See `deps/llvm/README` for details.

Then, it's all just regular make:

    $ make

Should give you some stuff in the `build` subdirectory.

## bin/hue

The `hue` program is a all-in-one tool which is essentially a REPL-able, JIT dynamic compiler.

    $ make hue
    $ build/bin/hue --help
    ...
    $ build/bin/hue
    → 42
    42
    → 42 * 8
    336
    → ^C
    $ build/bin/hue test/test_lang_data_literals.hue
    Hello World
    Hello World
    $ build/bin/hue -output-ir=- -compile-only test/test_lang_data_literals.hue
    # IR code here...
    $ build/bin/hue -parse-only test/test_lang_data_literals.hue
    # AST repr here...

You can chain hue with llvm tools in order to produce a machine-native program:

    $ build/bin/hue -output-ir=- -compile-only test/test_lang_data_literals.hue \
      | llvm-as -o=- | llvm-ld -native -Lbuild/lib -lhuert -o=program -
    $ ./program
    Hello World
    Hello World

If you don't have a local llvm installation, you might need to add the llvm bin directory from deps to your PATH environment variable before running the above.

    PATH=$PATH:$(pwd)/deps/llvm/bin/bin


## Objectives and plans

1. Have fun
2. Take is slowly and go bottom-up, analyzing machine code and drinking coffee
3. Functions that can capture its environment
4. Complex types (records/structs/et al) with automatic reference counting
5. Listen to Black Sabbath and take over the worlds

Seriously, this is just for fun. Don't expect anything from this project.

## Language

This section documents the Hue language.

### Comments
Comments are not really part of the language (yet), but rather part of the source file format.

    Comment = '#' <anything except a line break>* <LF>

A Comment starts with a '#' character and ends with a line break.

### Chunk
A unit of execution is called a chunk. Syntactically, a chunk is simply a *Block*.

Hue handles a chunk as the body of an anonymous function. As such, chunks can define local symbols. Chunks can also be (pre)compiled.

### Block
A Block is a list of expressions, which are executed sequentially, where the last expression in the block denotes the result value of the block itself.

    Block = Expression+

Expressions can be separated either by their natural boundaries or by line feeds (new-lines), in which case the indentation level is significant. Example:

    A
    B
      C
      D
    E

Here, A, B and E are part of the same block while C and D are part of a different sub-block. The indentation level of a block is defined by the start column of the first expression in the block.

### Expression
An Expression is the abstract basic language unit of Hue:

    Expression = BinaryOperation
               | '(' Expression ')'
               | Literal
               | Identifier
               | Function
               | Conditional
               | Structure

### BinaryOperation
An expression that takes two Expressions and one infix operator:

    BinaryOperation = Expression InfixOperator Expression

### InfixOperator
Denotes the operation of a BinaryOperation:

    InfixOperator = AssignmentOperator | ArithmeticOperator | ComparisonOperator
    AssignmentOperator = '='
    ArithmeticOperator = '*' | '/' | '+' | '-'
    ComparisonOperator = '<' | '>' | '<=' | '>=' | '!=' | '=='

### Literal
Literals are embedded data -- an essential part of Hue:

    Literal = NumberLiteral
            | BooleanLiteral
            | SequenceLiteral

    SequenceLiteral = DataLiteral | TextLiteral

### NumberLiteral
A number:

    NumberLiteral = IntegerLiteral ['.' IntegerLiteral]? NumberExponent?
    IntegerLiteral = DecIntegerLiteral | HexDecimalInteger
    NumberExponent = ['E' | 'e'] ['-' | '+']? DecIntegerLiteral
    DecIntegerLiteral = 0..9 [0..9 | _]*
    HexDecimalInteger = '0x' [0..9 | A..F | a..f | _]+

Internally Hue uses two different kinds of storage types for numbers:

- Integer numbers are stored as 64 bits where the first bit denotes if the number is negative or positive. The smallest possible value that can be stored is *-9_223_372_036_854_775_808* (*0x8000000000000000*) and the largest possible value is *9_223_372_036_854_775_807* (*0x7fffffffffffffff*).

- Fractional numbers are stored as [IEEE 754 double-precision floating-point](http://en.wikipedia.org/wiki/Double_precision_floating-point_format) values. There are *18_437_736_874_454_810_624* finite values. Half of them are negative and the other half positive. Beyond these values, precision decreases (exponent >1).

When an operation occurs between an integer and floating point number, the interger number is first promoted to a floating point equivalent. The operation then occurs with floating point operands. If the conversion is destructive (i.e. causing loss of precision), Hue will emit a warning when the encapsulating chunk is compiled.

### BooleanLiteral

    BooleanLiteral = 'true' | 'false'

Internally represented as a single-bit value. This is the result type of any comparison operation.

### SequenceLiteral
A literal sequence of items:

    SequenceLiteral = TextLiteral | DataLiteral

### TextLiteral
Unicode text:

    TextLiteral = '"' [RawCharacterLiteral | EncodedCharacterLiteral]* '"'
    RawCharacterLiteral = <Any Unicode character except '"' and '\'>
    EncodedCharacterLiteral = '\' ['"' | 't' | 'n' | 'r' | '\' | 'u' CharacterCode]
    CharacterCode = [0..9 | A..F | a..f | _]{1,8}

Text is stored as a sequence of 32-bit integers and treated according to the Unicode standard (UTF-32).

Examples:
    
    "Hello World"
    ""
    "ﾈ \u2192"


### DataLiteral
Raw string of bytes:

    DataLiteral = "'" [RawByteLiteral | EncodedByteLiteral]* "'"
    RawByteLiteral = <any octet except "'" and '\'>
    EncodedByteLiteral = '\' ["'" | 't' | 'n' | 'r' | '\' | 'x' ByteCode]
    ByteCode = [0..9 | A..F | a..f | _]{1,2}

Examples:

    'Hello World'
    ''
    'foo\n\x0bar'

### Identifier
A symbolic name that identifies a value:

    Identifier = IdentifierCharacter [0..9 | IdentifierCharacter]*
    IdentifierCharacter = '_' | A..Z | a..z | <U+80..U+FFFFFFFF>

Examples:

    foo
    _
    büz

### Function
A reusable executable unit:

    Function = 'func' FunctionParameters Block
    FunctionParameters = '(' Identifier* ')'

Example:

    func (a, b) a * b

### Conditional
Enables the program to flow into one of two different branches depending on a boolean condition:

    Conditional = 'if' Test BranchA 'else' BranchB
    Test = Expression
    BranchA = Block
    BranchB = Block

Example:

    if n > 5 100 else 200

### Structure
Packages several values together into a unit that can be passed around and referenced.

    Structure = 'struct' Block

Only assignment expressions are allowed to exist on the root level of the block. An assignment on the root level defines the symbol in the struct (rather than locally in the current scope).

Example:

    user = struct
      uid = 1
      about = struct
        name = "Rasmus Andersson"
        age = 29
    
    user:uid        # -> 1
    user:about:age  # -> 29

----

## License

See LICENSE for the standard MIT license.
