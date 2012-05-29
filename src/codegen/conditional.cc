// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "_VisitorImplHeader.h"

Value* Visitor::codegenConditional(const ast::Conditional *cond) {
  DEBUG_TRACE_LLVM_VISITOR;

  // Verify the conditional
  if (cond->testExpression() == 0) return error("Missing test expression in conditional");
  if (cond->trueBlock() == 0) return error("Missing true block in conditional");
  if (cond->falseBlock() == 0) return error("Missing false block in conditional");
  
  // Get enclosing function
  Function *F = builder_.GetInsertBlock()->getParent();

  // Codegen test expression
  Value* testV = codegen(cond->testExpression());
  if (testV == 0) return 0;

  // Cast test expression result to boolean if needed
  if (!testV->getType()->isIntegerTy(1)) {
    testV = castValueToBool(testV);
    if (testV == 0) return 0;
  }

  // Conditional result type
  Type* resultT = IRTypeForASTType(cond->resultType());
  if (resultT == 0) return 0;

  // True, false and end block
  BasicBlock *trueBB = BasicBlock::Create(getGlobalContext(), "then");
  BasicBlock *falseBB = BasicBlock::Create(getGlobalContext(), "else");
  BasicBlock *endBB = BasicBlock::Create(getGlobalContext(), "endif");

  // Conditional branch instruction. If testV is true, continue execution at trueBB.
  // Otherwise continue execution at falseBB.
  builder_.CreateCondBr(testV, trueBB, falseBB);

  // ------
  // Put the true block at the end of the current function and set it as insertion
  F->getBasicBlockList().push_back(trueBB);
  builder_.SetInsertPoint(trueBB);

  // Codegen the true block
  Value *trueV = codegenBlock(cond->trueBlock());
  if (trueV == 0) return 0;

  // Check result type and appempt cast if needed
  if (trueV->getType() != resultT) {
    trueV = castValueTo(trueV, resultT);
    if (trueV == 0) return 0;
  }

  // Terminate the true block with a "jump" to the "endif" block
  builder_.CreateBr(endBB);

  // Codegen of true block can change the current block, update trueBB for the PHI.
  trueBB = builder_.GetInsertBlock();


  // ------
  // Put the false block at the end of the current function and set it as insertion
  F->getBasicBlockList().push_back(falseBB);
  builder_.SetInsertPoint(falseBB);

  // Codegen the false block
  Value *falseV = codegenBlock(cond->falseBlock());
  if (falseV == 0) return 0;

  // Check result type and appempt cast if needed
  if (falseV->getType() != resultT) {
    falseV = castValueTo(falseV, resultT);
    if (falseV == 0) return 0;
  }

  // Terminate the false block with a "jump" to the "endif" block
  builder_.CreateBr(endBB);

  // Codegen of false block can change the current block, update trueBB for the PHI.
  falseBB = builder_.GetInsertBlock();

  // ------
  // Create endif block
  F->getBasicBlockList().push_back(endBB);
  builder_.SetInsertPoint(endBB);

  // Add PHI to the endif block
  PHINode *phi = builder_.CreatePHI(resultT, 2, "ifres");
  phi->addIncoming(trueV, trueBB);
  phi->addIncoming(falseV, falseBB);

  // Return the PHI
  return phi;
}

#include "_VisitorImplFooter.h"
