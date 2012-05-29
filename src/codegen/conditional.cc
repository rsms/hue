// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "_VisitorImplHeader.h"

class BlockAndValue {
public:
  explicit BlockAndValue(BasicBlock* B, Value* V) : block(B), value(V) {}
  BasicBlock* block;
  Value* value;
};

Value* Visitor::codegenConditional(const ast::Conditional *cond) {
  DEBUG_TRACE_LLVM_VISITOR;

  // Verify the conditional
  if (cond->branches().size() == 0) return error("Missing first branch in conditional");
  if (cond->defaultBlock() == 0) return error("Missing default branch in conditional");
  
  // Get enclosing function
  Function *F = builder_.GetInsertBlock()->getParent();
  
  // PHI setup
  // Our final "merge" block where we use a PHI to select the result from
  // the executed branch.
  BasicBlock *endBB = BasicBlock::Create(getGlobalContext(), "endif");
  // The blocks we create (which produce output) and that we will use in the PHI
  std::vector<BlockAndValue> blocksAndValues;
  // Most hi-fi type that we will use for the PHI
  Type* resultT = 0;
  
  
  BasicBlock *nextBB = BasicBlock::Create(getGlobalContext(), cond->branches().size() == 1 ? "else" : "elseif");
  
  // Generate conditional branches
  ast::Conditional::BranchList::const_iterator branchIT = cond->branches().begin();
  size_t i = 1;
  for (; branchIT != cond->branches().end(); ++branchIT, ++i) {
    const ast::Conditional::Branch& branch = *branchIT;
    //rlog("gen branch " << branch.toString());
    
    
    // ---- test ----
    // Generate code for the test expression
    Value* testV = codegen(branch.testExpression);
  
    // If the test expression is not already a boolean type, wrap it in the most
    // suitable comparator instruction.
    if (!testV->getType()->isIntegerTy(1)) {
      testV = castValueToBool(testV);
      if (testV == 0) return 0;
    }
    
    // Create the block that we will use for our "then" code. We need to create it
    // here since it needs to be referenced in the conditional branch instruction
    // below.
    BasicBlock *trueBB = BasicBlock::Create(getGlobalContext(), "iftrue");
  
    // Create conditional branch instruction.
    // Note: nextBB is the "else" block that we will generate in the next "while"
    //       iteration (or become the default "else" block).
    builder_.CreateCondBr(testV, trueBB, nextBB);
    //Value* condBr = builder_.CreateCondBr(testV, trueBB, nextBB);
    //rlog("Created test "); condBr->dump();
    
    
    // ---- true-block ----
    // Put the block at the end of the current function and set it as our insertion
    // point for upcoming instructions.
    F->getBasicBlockList().push_back(trueBB);
    builder_.SetInsertPoint(trueBB);
    
    // Build the "true" block
    Value *trueV = codegenBlock(branch.block);
    if (trueV == 0) return 0;
    builder_.SetInsertPoint(trueBB);
    // Create the "true" block terminator: A "jump" to the "endif" block
    builder_.CreateBr(endBB);
    // ---- /true-block ----
    
  
    // ---- save ----
    // Codegen of true-block can change the current block, update trueBB for the PHI.
    trueBB = builder_.GetInsertBlock();
    //rlog("Created iftrue "); trueBB->dump();
    
    // Update result type
    resultT = (resultT == 0) ? trueV->getType() : highestFidelityType(resultT, trueV->getType());
    if (resultT == 0) {
      return error(R_FMT("Incompatible types in conditional branches: "));
    }
    
    // We will eventually use the outcome of this block for the PHI in the endif block
    blocksAndValues.push_back(BlockAndValue(trueBB, trueV));
    // ---- /save ----
    
    
    // Set the insertion point for upcoming instructions to the next block.
    // This means that if we perform another "while" iteration after this, the test
    // expression will end up in this block. If we fall out of the "while" after this,
    // the default block will be generated into here.
    F->getBasicBlockList().push_back(nextBB);
    builder_.SetInsertPoint(nextBB);
    
    // Prepare for next block
    if (i < cond->branches().size())
      nextBB = BasicBlock::Create(getGlobalContext(), (i == cond->branches().size()-1) ? "else" : "elseif");
  }
  
  
  // Create default ("else") block
  // Build the "true" block
  Value *defaultV = codegenBlock(cond->defaultBlock());
  if (defaultV == 0) return 0;

  builder_.SetInsertPoint(nextBB);
  // Create the "default" block terminator: A "jump" to the "endif" block
  builder_.CreateBr(endBB);

  // Update result type
  resultT = highestFidelityType(resultT, defaultV->getType());
  if (resultT == 0) {
    // Unknown
    ast::Type* T1 = resultT ? ASTTypeForIRType(resultT) : 0;
    ast::Type* T2 = defaultV && defaultV->getType() ? ASTTypeForIRType(defaultV->getType()) : 0;
    return error(R_FMT("Incompatible types in conditional branches: "
                       << (T1 ? T1->toString() : "null") << " != "
                       << (T2 ? T2->toString() : "null") ));
  }
  
  // Save
  BasicBlock* defaultBB = builder_.GetInsertBlock();
  
  
  // Create endif block
  F->getBasicBlockList().push_back(endBB);
  builder_.SetInsertPoint(endBB);

  // Add PHI to the endif block
  PHINode *phi = builder_.CreatePHI(resultT, blocksAndValues.size()+1, "ifres");
  std::vector<BlockAndValue>::iterator it = blocksAndValues.begin();

  // Add each branch and its result to the PHI
  for (; it != blocksAndValues.end(); ++it) {
    BasicBlock* trueBB = it->block;
    Value* trueV = it->value;
    
    // Note: We can NOT cast values since the PHI must be the first instruction in the
    //       endif block.
    
    // Cast branch result value if needed
    //trueV = castValueTo(trueV, resultT);
    //if (trueV == 0)
    //  return error("Type mismatch of results from if..else branches");
    
    // Add result and block to PHI
    phi->addIncoming(trueV, trueBB);
  }
  
  // Add default result and block to PHI
  phi->addIncoming(defaultV, defaultBB);
  
  // Return the PHI
  return phi;
}

#include "_VisitorImplFooter.h"
