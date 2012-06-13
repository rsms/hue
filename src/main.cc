// Copyright (c) 2012, Rasmus Andersson. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file.

#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>

#include <llvm/Support/raw_ostream.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/PassManager.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <fcntl.h>

// from lli.cpp
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Type.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/CodeGen/LinkAllCodegenComponents.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/JITEventListener.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/PluginLoader.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Process.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/TargetSelect.h>
#include <cerrno>

#include "parse/FileInput.h"
#include "parse/Tokenizer.h"
#include "parse/TokenBuffer.h"
#include "parse/Parser.h"

#include "transform/LazyFuncResultTransformer.h"

#include "codegen/Visitor.h"

#include "Text.h"
#include "termstyle.h"
#include "linenoise/linenoise.h"

using namespace llvm;
using namespace hue;

namespace {
  cl::opt<std::string>
  InputFile(cl::desc("<input source>"), cl::Positional, cl::init("-"));

  cl::list<std::string>
  InputArgv(cl::ConsumeAfter, cl::desc("<program arguments>..."));

  cl::opt<bool> ForceInterpreter("force-interpreter",
                                 cl::desc("Force interpretation: disable JIT"),
                                 cl::init(false));

  cl::opt<bool> UseMCJIT(
    "use-mcjit", cl::desc("Enable use of the MC-based JIT (if available)"),
    cl::init(false));

  cl::opt<std::string> OutputIR(
    "output-ir", cl::desc("Generate and write IR to file at <path>."),
    cl::value_desc("path"),
    cl::init(" "));

  cl::opt<bool> OnlyParse("parse-only",
    cl::desc("Only parse the source, but do not compile or execute."),
    cl::init(false));

  cl::opt<bool> BatchMode("batch",
    cl::desc("Do NOT run in interactive (REPL) mode."),
    cl::init(false));

  cl::opt<bool> OnlyCompile("compile-only",
    cl::desc("Only compile the source, but do not execute."),
    cl::init(false));

  // Determine optimization level.
  cl::opt<char>
  OptLevel("O",
           cl::desc("Optimization level. [-O0, -O1, -O2, or -O3] "
                    "(default = '-O2')"),
           cl::Prefix,
           cl::ZeroOrMore,
           cl::init(' '));

  cl::opt<std::string>
  TargetTriple("mtriple", cl::desc("Override target triple for module"));

  cl::opt<std::string>
  MArch("march",
        cl::desc("Architecture to generate assembly for (see --version)"));

  cl::opt<std::string>
  MCPU("mcpu",
       cl::desc("Target a specific cpu type (-mcpu=help for details)"),
       cl::value_desc("cpu-name"),
       cl::init(""));

  cl::list<std::string>
  MAttrs("mattr",
         cl::CommaSeparated,
         cl::desc("Target specific attributes (-mattr=help for details)"),
         cl::value_desc("a1,+a2,-a3,..."));

  cl::opt<std::string>
  EntryFunc("entry-function",
            cl::desc("Specify the entry function (default = 'main') "
                     "of the executable"),
            cl::value_desc("function"),
            cl::init("main"));
  
  cl::opt<std::string>
  FakeArgv0("fake-argv0",
            cl::desc("Override the 'argv[0]' value passed into the executing"
                     " program"), cl::value_desc("executable"));
  
  cl::opt<bool>
  DisableCoreFiles("disable-core-files", cl::Hidden,
                   cl::desc("Disable emission of core files if possible"));

  cl::opt<bool>
  NoLazyCompilation("disable-lazy-compilation",
                  cl::desc("Disable JIT lazy compilation"),
                  cl::init(false));

  cl::opt<Reloc::Model>
  RelocModel("relocation-model",
             cl::desc("Choose relocation model"),
             cl::init(Reloc::Default),
             cl::values(
            clEnumValN(Reloc::Default, "default",
                       "Target default relocation model"),
            clEnumValN(Reloc::Static, "static",
                       "Non-relocatable code"),
            clEnumValN(Reloc::PIC_, "pic",
                       "Fully relocatable, position independent code"),
            clEnumValN(Reloc::DynamicNoPIC, "dynamic-no-pic",
                       "Relocatable external references, non-relocatable code"),
            clEnumValEnd));

  cl::opt<llvm::CodeModel::Model>
  CMModel("code-model",
          cl::desc("Choose code model"),
          cl::init(CodeModel::JITDefault),
          cl::values(clEnumValN(CodeModel::JITDefault, "default",
                                "Target default JIT code model"),
                     clEnumValN(CodeModel::Small, "small",
                                "Small code model"),
                     clEnumValN(CodeModel::Kernel, "kernel",
                                "Kernel code model"),
                     clEnumValN(CodeModel::Medium, "medium",
                                "Medium code model"),
                     clEnumValN(CodeModel::Large, "large",
                                "Large code model"),
                     clEnumValEnd));

}


// Parse text into a Hue expression
ast::Block* parse(const Text& text, const Text& sourceName) {
  // A tokenizer produce tokens parsed from a ByteInput
  Tokenizer tokenizer(text);
  
  // A TokenBuffer reads tokens from a Tokenizer and maintains limited history
  TokenBuffer tokens(tokenizer);
  
  // A parser reads the token buffer and produce an AST
  Parser parser(tokens);

  // Block to hold the expression(s) we will parse
  ast::Block* block = new ast::Block(&NilType);

  // Parse all available expressions
  while (!parser.end()) {
    Expression *expr = parser.parseExpression(true);
    if (expr == 0) {
      if (parser.end()) {
        break;
      }
      //errs() << "Failed to parse expression in " << sourceName.UTF8String() << "\n";
      delete block;
      return 0;
    }
    block->addExpression(expr);
  }

  // Check for errors
  if (parser.errors().size() != 0) {
    //std::cerr << parser.errors().size() << " parse error(s)." << std::endl;
    delete block;
    return 0;
  }

  // Apply transformations

  // Apply lazy function result transformer. Updates the AST and resolves any
  // unknown function result types.
  transform::LazyFuncResultTransformer LFR(block);
  std::string ErrorMsg;
  if (!LFR.run(ErrorMsg)) {
    std::cerr << "Parse error: " << ErrorMsg << std::endl;
    delete block;
    return 0;
  }

  return block;
}


void repl_completion(const char *buf, linenoiseCompletions *lc) {
  // if (buf[0] == 'h') {
  //   linenoiseAddCompletion(lc, "hello");
  //   linenoiseAddCompletion(lc, "hello there");
  // }
}


void repl(llvm::Module *Mod, llvm::FunctionPassManager* funcPassManager, llvm::ExecutionEngine *EE) {
  ast::Block* moduleBlock = 0;
  unsigned long inputCounter = 0;

  // Setup linenoise
  const char* historyFilename = ".hue_history";
  linenoiseSetCompletionCallback(repl_completion);
  linenoiseHistorySetMaxLen(1000);
  linenoiseHistoryLoad(historyFilename);

  // Create code generator
  codegen::Visitor codegen;
  codegen.setModule(Mod);

  // Run static constructors.
  errno = 0;
  EE->runStaticConstructorsDestructors(Mod, false);

  char* inputBytes = 0;
  const char* prompt = "> ";
  if (1) { // is color terminal
    prompt = TS_Yellow "→" TS_None " ";
  }

  repl_loop:
  while (!feof(stdin)) {
    // Name this iteration
    char replIterationName[100];
    snprintf(replIterationName, 100, "repl#%lu", ++inputCounter);

    // Read input
    if (inputBytes != 0) free(inputBytes);
    inputBytes = linenoise(prompt);
    if (inputBytes == 0) break; // EOF
    if (inputBytes[0] == '\0') continue; // empty

    // Check if empty so to avoid recording empty entries in history
    size_t i = 0, len = strlen(inputBytes);
    for (; i < len && (inputBytes[i] == ' ' || inputBytes[i] == '\t'); ++i) {}
    if (i == len) continue; // empty

    // Parse into text as UTF-8 data
    Text input(inputBytes);

    // Record the input into history
    if (linenoiseHistoryAdd(inputBytes, 0))
      linenoiseHistorySave(historyFilename);

    // Parse
    std::cerr << TS_Brown "** Parsing input '" << input << "'" TS_None << std::endl;
    if (moduleBlock != 0) delete moduleBlock;
    moduleBlock = parse(input, "<stdin>");
    if (moduleBlock == 0) {
      errs() << TS_Brown "** parse() failed.\n" TS_None;
      goto repl_loop;
    }

    // If we got no expressions, we are done with this iteration
    if (moduleBlock->expressions().size() == 0) {
      errs() << TS_Light_Red "Error: Empty expression\n" TS_None;
      goto repl_loop;
    }

    // Print what we parsed
    // TODO: Make this toggle-able
    outs() << TS_Dark_Gray << moduleBlock->toString() << TS_None "\n";
    // for (ast::ExpressionList::const_iterator I = moduleBlock->expressions().begin(),
    //       E = moduleBlock->expressions().end(); I != E; ++I) {
    //   std::cout << TS_Dark_Gray << (*I)->toString() << TS_None << std::endl;
    // }

    // Wrap parsed block in a function
    ast::Function* moduleFunc = Parser::wrapBlockInFunction(moduleBlock);
    //outs() << moduleFunc->toString() << "\n";

    // Generate code
    errs() << TS_Brown "** Generating code\n" TS_None;
    llvm::Function* moduleF = (llvm::Function*)codegen.codegenFunction(moduleFunc, "", replIterationName);
    if (moduleF == 0) {
      //errs() << TS_Light_Red << codegen.errors().size() << " error(s) during code generation.\n" TS_None;
      goto repl_loop;
    }

    // Apply optimizations
    if (funcPassManager) {
      // Execute all of the passes scheduled for execution. Keep track of
      // whether any of the passes modifies the module, and if so, return true.
      funcPassManager->run(*moduleF);
    }

    // Print generated code
    // TODO: Make this toggle-able
    outs() << TS_Dark_Gray;
    //moduleF->print(outs(), 0);
    for (llvm::Function::const_iterator I = moduleF->begin(), E = moduleF->end(); I != E; ++I) {
      (*I).print(outs(), 0);
    }
    outs() << TS_None;

    // Execute
    errs() << TS_Brown "** Executing\n" TS_None;
    std::vector<llvm::GenericValue> args;
    llvm::GenericValue returnV = EE->runFunction(moduleF, args);

    // Find out what the function returns and print a representation
    const ast::Type* resultType = moduleFunc->resultType();
    std::cout << TS_Light_Blue;
    switch (resultType->typeID()) {
      case ast::Type::Nil:
        std::cout << TS_Dark_Gray "nil"; break;
      
      case ast::Type::Float:
        std::cout << returnV.DoubleVal; break;

      case ast::Type::Int: {
        std::cout << (int64_t)returnV.IntVal.getLimitedValue();
        break;
      }

      case ast::Type::Char:
        std::cout << (uint32_t)returnV.IntVal.getLimitedValue(UINT32_MAX); break;

      case ast::Type::Byte:
        std::cout << (uint8_t)returnV.IntVal.getLimitedValue(UINT8_MAX); break;

      case ast::Type::Bool:
        std::cout << (returnV.IntVal.getLimitedValue(1) ? "true" : "false"); break;

      case ast::Type::Func: {
        ast::Expression* lastExpr = moduleBlock->expressions().back();
        // TODO: Represent the actual result
        std::cout << TS_Brown << lastExpr->toString();
        break;
      }

      case ast::Type::Array: {
        // TODO: Represent an array
        std::cout << TS_Cyan << "[" << returnV.PointerVal << "]"; break;
      }

      // TODO: case ast::Type::Named:
      
      default:
        std::cout << "?"; break;
    }
    std::cout << TS_None << std::endl;


    // Clean up
    EE->freeMachineCodeForFunction(moduleF);
    moduleF->removeFromParent();
  }

  if (inputBytes != 0)
    free(inputBytes);

  codegen.reset(); // todo: use this when we reset our REPL env

  // Run static destructors.
  EE->runStaticConstructorsDestructors(Mod, true);
}


int main(int argc, char **argv, char * const *envp) {
  sys::PrintStackTraceOnErrorSignal();
  PrettyStackTraceProgram X(argc, argv);
  
  LLVMContext &Context = getGlobalContext();
  atexit(llvm_shutdown);  // Call llvm_shutdown() on exit.

  // If we have a native target, initialize it to ensure it is linked in and
  // usable by the JIT.
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();

  cl::ParseCommandLineOptions(argc, argv, "Hue interpreter & dynamic compiler\n");

  // If the user doesn't want core files, disable them.
  if (DisableCoreFiles)
    sys::Process::PreventCoreFiles();
  
  std::string ErrorMsg;
  ast::Block* moduleBlock = 0;

  // Force batch mode if we are given any source files
  if (!InputFile.empty() && InputFile != "-") {
    BatchMode = true;
  }


  if (BatchMode) {
    // Read input file
    Text textSource;
    if (!textSource.setFromUTF8FileOrSTDIN(InputFile.c_str(), ErrorMsg)) {
      std::cerr << "Failed to open input file: " << ErrorMsg << std::endl;
      return 1;
    }
    
    // Parse
    if ((moduleBlock = parse(textSource, InputFile)) == 0)
      return 1;

    // Only parse? Then we are done.
    if (OnlyParse) {
      std::cout << moduleBlock->toString() << std::endl;
      return 0;
    }

  }


  // Argv and module name
  std::string moduleName;
  if (!FakeArgv0.empty()) {
    // If the user specifically requested an argv[0] to pass into the program,
    // do it now.
    moduleName = InputFile = FakeArgv0;
  } else {
    // Otherwise, if there is a .hue suffix on the executable strip it off
    if (StringRef(InputFile).endswith(".hue"))
      InputFile.erase(InputFile.length() - 4);
    moduleName = InputFile;
  }
  // Add the module's name to the start of the vector of arguments to main().
  InputArgv.insert(InputArgv.begin(), InputFile);


  // LLVM module
  llvm::Module *Mod = new Module(moduleName, Context);



  // Create ExecutionEngine
  EngineBuilder builder(Mod);
  builder.setMArch(MArch);
  builder.setMCPU(MCPU);
  builder.setMAttrs(MAttrs);
  builder.setRelocationModel(RelocModel);
  builder.setCodeModel(CMModel);
  builder.setErrorStr(&ErrorMsg);
  builder.setEngineKind(ForceInterpreter
                        ? EngineKind::Interpreter
                        : EngineKind::JIT);

  // If we are supposed to override the target triple, do so now.
  if (!TargetTriple.empty())
    Mod->setTargetTriple(Triple::normalize(TargetTriple));

  // Enable MCJIT, if desired.
  if (UseMCJIT)
    builder.setUseMCJIT(true);

  CodeGenOpt::Level OLvl = CodeGenOpt::Default;
  switch (OptLevel) {
  default:
    errs() << argv[0] << ": invalid optimization level.\n";
    return 1;
  case ' ': break;
  case '0': OLvl = CodeGenOpt::None; break;
  case '1': OLvl = CodeGenOpt::Less; break;
  case '2': OLvl = CodeGenOpt::Default; break;
  case '3': OLvl = CodeGenOpt::Aggressive; break;
  }
  builder.setOptLevel(OLvl);

  ExecutionEngine *EE = builder.create();
  if (!EE) {
    if (!ErrorMsg.empty())
      errs() << argv[0] << ": error creating EE: " << ErrorMsg << "\n";
    else
      errs() << argv[0] << ": unknown error creating EE!\n";
    return 1;
  }

  EE->RegisterJITEventListener(createOProfileJITEventListener());



  // Setup optimization pass manager
  PassManager* modPassManager = 0;
  FunctionPassManager* funcPassManager = 0;
  PassManagerBase* passManager = 0;

  if (OLvl != CodeGenOpt::None) {
    if (!BatchMode) {
      // In REPL mode, we apply passes to functions, since that's our largest chunk
      // of work.
      passManager = funcPassManager = new FunctionPassManager(Mod);
    } else {
      // In batch mode, our largest chunk is the complete module, so we apply passes
      // to complete modules.
      passManager = modPassManager = new PassManager();
    }

    // Set up the optimizer pipeline.
    // Start with registering info about how the target lays out data structures.
    passManager->add(new TargetData(*EE->getTargetData()));

    // Provide basic AliasAnalysis support for GVN.
    passManager->add(createBasicAliasAnalysisPass());

    // merge duplicate global constants together into a single constant that is shared.
    // This is useful because some passes (ie TraceValues) insert a lot of string
    // constants into the program, regardless of whether or not they duplicate an
    // existing string.
    if (modPassManager)
      passManager->add(createConstantMergePass());
    
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    passManager->add(createInstructionCombiningPass());

    // Remove redundant instructions.
    passManager->add(createInstructionSimplifierPass());
    
    // Reassociate expressions.
    passManager->add(createReassociatePass());
    
    // Eliminate Common SubExpressions.
    passManager->add(createGVNPass());

    // uses a heuristic to inline direct function calls to small functions.
    if (modPassManager)
      passManager->add(createFunctionInliningPass());
    
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    passManager->add(createCFGSimplificationPass());

    // This pass reorders basic blocks in order to increase the
    // number of fall-through conditional branches.
    passManager->add(createBlockPlacementPass());

    // This pass is more powerful than DeadInstElimination,
    // because it is worklist driven that can potentially revisit instructions when
    // their other instructions become dead, to eliminate chains of dead
    // computations.
    passManager->add(createDeadCodeEliminationPass());

    // This pass eliminates call instructions to the current
    // function which occur immediately before return instructions.
    passManager->add(createTailCallEliminationPass());

    // discovers functions that do not access memory, or only read memory, and gives
    // them the readnone/readonly attribute. It also discovers function arguments
    // that are not captured by the function and marks them with the nocapture
    // attribute.
    if (modPassManager)
      passManager->add(createFunctionAttrsPass());
  }



  if (!BatchMode) {
    if (NoLazyCompilation) {
      errs() << argv[0] << ": Can not disable lazy compilation in REPL mode\n";
    }

    // Enable lazy compilation
    EE->DisableLazyCompilation(NoLazyCompilation);

    // Enter REPL
    repl(Mod, funcPassManager, EE);

  } else {
    // Batch mode (not REPL)

    // XXX dump parsed module block to stdout
    std::cout << moduleBlock->toString() << std::endl;

    // Make sure the module block results in an integer value, since we will
    // call it as a standard main function.
    // TODO: Move this to where we call runFunctionAsMain
    if (!moduleBlock->resultType()->isInt()) {
      moduleBlock->addExpression(new ast::IntLiteral("0"));
    }

    // Module wrapper function
    ast::Function* moduleFunc = Parser::wrapBlockInFunction(moduleBlock);

    // Create code generator
    codegen::Visitor codegen;
    codegen.setModule(Mod);

    // Generate code
    llvm::Function* moduleF = (llvm::Function*)codegen.codegenFunction(moduleFunc, "", moduleName);
    if (moduleF == 0) {
      std::cerr << codegen.errors().size() << " error(s) during code generation." << std::endl;
      return 1;
    }
    codegen.reset();
  
    // Apply optimizations
    if (modPassManager) {
      // Execute all of the passes scheduled for execution. Keep track of
      // whether any of the passes modifies the module, and if so, return true.
      modPassManager->run(*Mod);
    }

    // Output IR (-output-ir)
    if (OutputIR != " ") {
      if (OutputIR.empty() || OutputIR == "-") {
        Mod->print(outs(), 0);
      } else {
        llvm::raw_fd_ostream os(OutputIR.c_str(), ErrorMsg);
        if (os.has_error()) {
          std::cerr << "Failed to open file for output: " << ErrorMsg << std::endl;
          return 1;
        }
        Mod->print(os, 0);
      }
    }

    // Only compile? Then we are done. (-compile-only)
    if (OnlyCompile)
      return 0;

    // If not jitting lazily, load the whole bitcode file eagerly too.
    if (NoLazyCompilation) {
      if (Mod->MaterializeAllPermanently(&ErrorMsg)) {
        errs() << argv[0] << ": bitcode didn't read correctly.\n";
        errs() << "Reason: " << ErrorMsg << "\n";
        exit(1);
      }
      EE->DisableLazyCompilation(NoLazyCompilation);
    }

    // If the program doesn't explicitly call exit, we will need the Exit 
    // function later on to make an explicit call, so get the function now. 
    Constant *Exit = Mod->getOrInsertFunction("exit", llvm::Type::getVoidTy(Context),
                                                      llvm::Type::getInt32Ty(Context),
                                                      NULL);

    // Reset errno to zero on entry to main.
    errno = 0;
   
    // Run static constructors.
    EE->runStaticConstructorsDestructors(false);

    if (NoLazyCompilation) {
      for (Module::iterator I = Mod->begin(), E = Mod->end(); I != E; ++I) {
        llvm::Function *Fn = &*I;
        if (Fn != moduleF && !Fn->isDeclaration())
          EE->getPointerToFunction(Fn);
      }
    }

    // Run main.
    int Result = EE->runFunctionAsMain(moduleF, InputArgv, envp);

    // Run static destructors.
    EE->runStaticConstructorsDestructors(true);
    
    // If the program didn't call exit explicitly, we should call it now. 
    // This ensures that any atexit handlers get called correctly.
    if (llvm::Function *ExitF = dyn_cast<llvm::Function>(Exit)) {
      std::vector<GenericValue> Args;
      GenericValue ResultGV;
      ResultGV.IntVal = APInt(32, Result);
      Args.push_back(ResultGV);
      EE->runFunction(ExitF, Args);
      errs() << "ERROR: exit(" << Result << ") returned!\n";
      abort();
    } else {
      errs() << "ERROR: exit defined with wrong prototype!\n";
      abort();
    }

  }


  // Call the main function from M as if its signature were:
  //   int main (int argc, char **argv, const char **envp)
  // using the contents of Args to determine argc & argv, and the contents of
  // EnvVars to determine envp.
  //
  // llvm::Function *EntryFn = Mod->getFunction(EntryFunc);
  // if (!EntryFn) {
  //   errs() << '\'' << EntryFunc << "\' function not found in module.\n";
  //   return -1;
  // }


  

  
  //
  // From here:
  //
  //   PATH=$PATH:$(pwd)/deps/llvm/bin/bin
  //
  //   Run the generated program:
  //     lli out.ll
  //
  //   Generate target assembler:
  //     llc -asm-verbose -o=- out.ll
  //
  //   Generate target image:
  //     llvm-as -o=- out.ll | llvm-ld -native -o=out.a -
  //
  // See http://llvm.org/docs/CommandGuide/ for details on these tools.
  //
  
  return 0;
}