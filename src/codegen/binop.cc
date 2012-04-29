#include "_VisitorImplHeader.h"

Value *Visitor::codegenBinaryExpression(ast::BinaryExpression *binExpr) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  ast::Expression *lhs = binExpr->lhs();
  ast::Expression *rhs = binExpr->rhs();
  
  Value *L = codegen(lhs); if (L == 0) return 0;
  Value *R = codegen(rhs); if (R == 0) return 0;
  
  Type* LT = L->getType();
  Type* RT = R->getType();
  Type* type = LT;
  
  // Numbers: cast to richest type if needed
  if (LT->isIntegerTy() && RT->isDoubleTy()) {
    // Cast L to double
    L = builder_.CreateSIToFP(L, (type = RT), "itoftmp");
  } else if (LT->isDoubleTy() && RT->isIntegerTy()) {
    // Cast R to double
    R = builder_.CreateSIToFP(R, (type = LT), "itoftmp");
  }

  // std::cerr << "L: "; if (!L) std::cerr << "<nil>" << std::endl; else L->dump();
  // std::cerr << "R: "; if (!R) std::cerr << "<nil>" << std::endl; else R->dump();
  // 
  // LT = L->getType();
  // RT = R->getType();
  // std::cerr << "LT: "; if (!LT) std::cerr << "<nil>" << std::endl; else {
  //   LT->dump(); std::cerr << std::endl; }
  // std::cerr << "RT: "; if (!RT) std::cerr << "<nil>" << std::endl; else {
  //   RT->dump(); std::cerr << std::endl; }

  switch (binExpr->operatorValue()) {
    
    case '+': {
      if (type->isIntegerTy()) {
        return builder_.CreateAdd(L, R, "addtmp");
      } else if (type->isDoubleTy()) {
        return builder_.CreateFAdd(L, R, "addtmp");
      } else {
        break;
      }
    }
    
    case '-': {
      if (type->isIntegerTy()) {
        return builder_.CreateSub(L, R, "subtmp");
      } else if (type->isDoubleTy()) {
        return builder_.CreateFSub(L, R, "subtmp");
      } else {
        break;
      }
    }
    
    case '*': {
      if (type->isIntegerTy()) {
        return builder_.CreateMul(L, R, "multmp");
      } else if (type->isDoubleTy()) {
        return builder_.CreateFMul(L, R, "multmp");
      } else {
        break;
      }
    }
    
    case '<': {
      if (type->isIntegerTy()) {
        return builder_.CreateICmpULT(L, R, "cmptmp");
        //L = builder_.CreateFCmpULT(L, R, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        //return builder_.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "booltmp");
      } else if (type->isDoubleTy()) {
        return builder_.CreateFCmpULT(L, R, "cmptmp");
        //L = builder_.CreateFCmpULT(L, R, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        //return builder_.CreateUIToFP(L, Type::getDoubleTy(getGlobalContext()), "booltmp");
      } else {
        break;
      }
      
    }
    default: break;
  }
  
  return error("invalid binary operator");
}

#include "_VisitorImplFooter.h"
