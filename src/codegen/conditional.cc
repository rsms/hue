#include "_VisitorImplHeader.h"

Value* Visitor::codegenConditional(const ast::Conditional *cond) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  // Generate code for the test expression
  Value* testV = codegen(cond->testExpression());
  
  // If the test expression is not already a boolean type, wrap it in the most
  // suitable comparator instruction.
  if (!testV->getType()->isIntegerTy(1)) {
    testV = castValueToBool(testV);
    if (testV == 0) return 0;
  }
  
  // Get enclosing function
  Function *F = builder_.GetInsertBlock()->getParent();
  
  // Create blocks for the then and else cases.  Insert the 'then' block at the
  // end of the function.
  BasicBlock *trueBB = BasicBlock::Create(getGlobalContext(), "iftrue", F);
  BasicBlock *falseBB = BasicBlock::Create(getGlobalContext(), "iffalse");
  BasicBlock *mergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");
  
  // Create conditional branch instruction
  builder_.CreateCondBr(testV, trueBB, falseBB);
  
  // Build the "true" block
  builder_.SetInsertPoint(trueBB);
  Value *trueV = codegenBlock(cond->trueBlock());
  if (trueV == 0) return 0;
  builder_.CreateBr(mergeBB);
  
  // Codegen of 'trueBlock' can change the current block, update trueBB for the PHI.
  trueBB = builder_.GetInsertBlock();
  
  // Push the "false" block to the end of the function and build it
  F->getBasicBlockList().push_back(falseBB);
  builder_.SetInsertPoint(falseBB);
  Value *falseV = codegenBlock(cond->falseBlock());
  if (falseV == 0) return 0;
  builder_.CreateBr(mergeBB);
  
  // Codegen of 'Else' can change the current block, update ElseBB for the PHI.
  falseBB = builder_.GetInsertBlock();
  
  // Create merge block
  F->getBasicBlockList().push_back(mergeBB);
  builder_.SetInsertPoint(mergeBB);
  
  // Assure the results from the different branches are of the same type.
  Type* resultT = trueV->getType();
  if (resultT != falseV->getType()) {
    // Find the best type
    resultT = highestFidelityType(resultT, falseV->getType());
    
    // Attempt cast
    if (resultT != 0) {
      trueV = castValueTo(trueV, resultT);
      falseV = castValueTo(falseV, resultT);
    }
    
    if (resultT == 0 || trueV == 0 || falseV == 0)
      return error("Type mismatch of results from if..else branches");
  }
  
  // Create a PHI for the resulting values
  PHINode *PN = builder_.CreatePHI(trueV->getType(), 2, "iftmp");
  PN->addIncoming(trueV, trueBB);
  PN->addIncoming(falseV, falseBB);
  
  return PN;
}

#include "_VisitorImplFooter.h"
