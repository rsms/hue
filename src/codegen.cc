#include "Node.h"
#include "codegen.h"
#include "parser.hh"

#include <vector>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

using namespace std;
using namespace llvm;

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root) {
  // Type Definitions
  std::vector<Type*>FuncTy_0_args;
  FuncTy_0_args.push_back(IntegerType::get(module()->getContext(), 32));
  PointerType* PointerTy_2 = PointerType::get(IntegerType::get(module()->getContext(), 8), 0);
  PointerType* PointerTy_1 = PointerType::get(PointerTy_2, 0);
  FuncTy_0_args.push_back(PointerTy_1);
  FunctionType* FuncTy_0 = FunctionType::get(
    /*Result=*/IntegerType::get(module()->getContext(), 32),
    /*Params=*/FuncTy_0_args,
    /*isVarArg=*/false);
 
  // Function Declarations
  Function* func_main = module()->getFunction("main");
  if (!func_main) {
    func_main = Function::Create(
      /*Type=*/FuncTy_0,
      /*Linkage=*/GlobalValue::ExternalLinkage,
      /*Name=*/"main", module());
    func_main->setCallingConv(CallingConv::C);
  }
  AttrListPtr func_main_PAL;
  {
    SmallVector<AttributeWithIndex, 4> Attrs;
    AttributeWithIndex PAWI;
    PAWI.Index = 2U; PAWI.Attrs = 0  | Attribute::NoCapture;
    Attrs.push_back(PAWI);
    PAWI.Index = 4294967295U; PAWI.Attrs = 0  | Attribute::NoUnwind | Attribute::ReadNone | Attribute::UWTable;
    Attrs.push_back(PAWI);
    func_main_PAL = AttrListPtr::get(Attrs.begin(), Attrs.end());
  }
  func_main->setAttributes(func_main_PAL);
 
  // Global Variable Declarations
 
 
  // Constant Definitions
  ConstantInt* const_int32_3 = ConstantInt::get(module()->getContext(), APInt(32, StringRef("0"), 10));
 
  // Global Variable Definitions
 
  // Function Definitions
 
  // Function: main (func_main)
  {
    Function::arg_iterator args = func_main->arg_begin();
    Value* int32_argc = args++;
    int32_argc->setName("argc");
    Value* ptr_argv = args++;
    ptr_argv->setName("argv");
  
    const char *block_label = "";
    BasicBlock* mainBlock = BasicBlock::Create(module()->getContext(), block_label, func_main, 0);
    pushBlock(mainBlock);
    root.codeGen(*this);
    ReturnInst::Create(module()->getContext(), const_int32_3, mainBlock);
    popBlock();
  
    // BasicBlock* label_4 = BasicBlock::Create(module()->getContext(), "", func_main, 0);
    // ReturnInst::Create(module()->getContext(), const_int32_3, label_4);
  }
  
  std::cout << "Code is generated.\n";
	PassManager pm;
	pm.add(createPrintModulePass(&outs()));
	pm.run(*module());
}


// Returns an LLVM type based on the identifier
Type *CodeGenContext::toLLVMType(const NIdentifier& type) {
	if (type.name.compare("int") == 0) {
		return Type::getInt64Ty(module()->getContext());
	} else if (type.name.compare("double") == 0) {
		return Type::getDoubleTy(module()->getContext());
	}
	return Type::getVoidTy(module()->getContext());
}


// Executes the AST by running the main function
GenericValue CodeGenContext::runCode() {
	std::cout << "Running code...\n";

  // namespace CodeGenOpt {
  //   enum Level {
  //     None,        // -O0
  //     Less,        // -O1
  //     Default,     // -O2, -Os
  //     Aggressive   // -O3
  //
  ExecutionEngine *ee = ExecutionEngine::create(module(),
                                                /*bool ForceInterpreter =*/ false,
                                                /*std::string *ErrorStr =*/ 0,
                                                /*CodeGenOpt::Level OptLevel =*/ CodeGenOpt::None,
                                                /*bool GVsWithCode =*/ true);
  vector<GenericValue> noargs;
  
  // ConstantInt* const_int32_0 = ConstantInt::get(module()->getContext(), APInt(32, StringRef("0"), 10));
  // std::vector<GenericValue> params;
  // GenericValue v0;
  // v0.IntVal = 0L;
  // params.push_back(v0);
  // params.push_back(v0);
  
  GenericValue returnValue;
  
  Function* func_main = module()->getFunction("main");
  if (!func_main) {
    std::cout << "Can't find main function\n";
  } else {
    returnValue = ee->runFunction(func_main, noargs);
    std::cout << "Code was run.\n";
  }

  return returnValue;
}


Value* NBlock::codeGen(CodeGenContext& context) {
	StatementList::const_iterator it;
	Value *last = NULL;
	for (it = statements.begin(); it != statements.end(); it++) {
		std::cout << "Generating code for " << typeid(**it).name() << std::endl;
		last = (**it).codeGen(context);
	}
	std::cout << "Creating block" << std::endl;
	return last;
}


Value* NVariableDeclaration::codeGen(CodeGenContext& context) {
	std::cout << "Creating variable declaration " << type.name << " " << id.name << std::endl;
	AllocaInst *alloc = new AllocaInst(context.toLLVMType(type), id.name.c_str(), context.currentBlock());
	context.locals()[id.name] = alloc;
	if (assignmentExpr != NULL) {
		NAssignment assn(id, *assignmentExpr);
		assn.codeGen(context);
	}
	return alloc;
}

// -- Code Generation --

Value* NInteger::codeGen(CodeGenContext& context) {
	std::cout << "Creating integer: " << value << std::endl;
	return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context) {
	std::cout << "Creating double: " << value << std::endl;
	return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value);
}

Value* NIdentifier::codeGen(CodeGenContext& context) {
	std::cout << "Creating identifier reference: " << name << std::endl;
	if (context.locals().find(name) == context.locals().end()) {
		std::cerr << "undeclared variable " << name << std::endl;
		return NULL;
	}
	return new LoadInst(context.locals()[name], "", false, context.currentBlock());
}

Value* NBinaryOperator::codeGen(CodeGenContext& context) {
	std::cout << "Creating binary operation " << op << std::endl;
	Instruction::BinaryOps instr;
	switch (op) {
		case TPLUS: 	instr = Instruction::Add; goto math;
		case TMINUS: 	instr = Instruction::Sub; goto math;
		case TMUL: 		instr = Instruction::Mul; goto math;
		case TDIV: 		instr = Instruction::SDiv; goto math;

		// TODO comparison
	}

	return NULL;
math:
	return BinaryOperator::Create(instr, lhs.codeGen(context),
		rhs.codeGen(context), "", context.currentBlock());
}

Value* NAssignment::codeGen(CodeGenContext& context) {
	std::cout << "Creating assignment for " << lhs.name << std::endl;
	if (context.locals().find(lhs.name) == context.locals().end()) {
		std::cerr << "undeclared variable " << lhs.name << std::endl;
		return NULL;
	}
	return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());
}

Value* NExpressionStatement::codeGen(CodeGenContext& context) {
	std::cout << "Generating code for " << typeid(expression).name() << std::endl;
	return expression.codeGen(context);
}


Value* NFunctionDeclaration::codeGen(CodeGenContext& context) {
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		argTypes.push_back(context.toLLVMType((**it).type));
	}
	
  //static llvm::FunctionType* llvm::FunctionType::get(llvm::Type*, llvm::ArrayRef<llvm::Type*>, bool)
  //static llvm::FunctionType* llvm::FunctionType::get(llvm::Type*, bool)
	FunctionType* ftype = FunctionType::get(
    /*Result=*/context.toLLVMType(type),
    /*Params=*/argTypes,
    /*isVarArg=*/false);
	
	Function *function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.module());
	const char *block_label = "";
	BasicBlock *bblock = BasicBlock::Create(context.module()->getContext(), "entry", function, 0);

	context.pushBlock(bblock);

	for (it = arguments.begin(); it != arguments.end(); it++) {
		(**it).codeGen(context);
	}

	block.codeGen(context);
	ReturnInst::Create(context.module()->getContext(), bblock);

	context.popBlock();
	std::cout << "Creating function: " << id.name << std::endl;
	return function;
}

Value* NFunctionCall::codeGen(CodeGenContext& context) {
  // Find function
  Function *function = context.module()->getFunction(id.name.c_str());
  if (function == NULL) {
    std::cerr << "no such function " << id.name << std::endl;
  }
  
  // Make argument list
  std::vector<Value*> args;
  ExpressionList::const_iterator it;
  for (it = arguments.begin(); it != arguments.end(); it++) {
    args.push_back((**it).codeGen(context));
  }
  
  // Create call instruction
  // llvm::CallInst* llvm::CallInst::Create(llvm::Value*, llvm::ArrayRef<llvm::Value*>, const llvm::Twine&, llvm::Instruction*)
  // llvm::CallInst* llvm::CallInst::Create(llvm::Value*, llvm::ArrayRef<llvm::Value*>, const llvm::Twine&, llvm::BasicBlock*)
  // llvm::CallInst* llvm::CallInst::Create(llvm::Value*, const llvm::Twine&, llvm::Instruction*)
  // llvm::CallInst* llvm::CallInst::Create(llvm::Value*, const llvm::Twine&, llvm::BasicBlock*)
  CallInst *call = CallInst::Create(function, args, "", context.currentBlock());
  call->setCallingConv(CallingConv::C);
  call->setTailCall(false);
  
  std::cout << "Creating method call: " << id.name << std::endl;
  return call;
}
