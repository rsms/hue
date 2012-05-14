// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#include "_VisitorImplHeader.h"

Value *Visitor::codegenBinaryOp(const ast::BinaryOp *binOpExp) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  ast::Expression *lhs = binOpExp->lhs();
  ast::Expression *rhs = binOpExp->rhs();
  
  Value *L = codegen(lhs); if (L == 0) return 0;
  Value *R = codegen(rhs); if (R == 0) return 0;
  
  Type* LT = L->getType();
  Type* RT = R->getType();
  Type* type = LT;
  
  // Numbers: cast to richest type if needed
  if (LT->isIntegerTy() && RT->isDoubleTy()) {
    // Cast L to double
    rlog("Binop: Cast LHS to double");
    L = builder_.CreateSIToFP(L, (type = RT), "itoftmp");
  } else if (LT->isDoubleTy() && RT->isIntegerTy()) {
    // Cast R to double
    rlog("Binop: Cast RHS to double. LHS -> "); L->dump();
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

  switch (binOpExp->operatorValue()) {
    
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
    
    case '/': {
      // UDiv: unsigned integer division
      // SDiv: signed integer division
      // FDiv: floating point division
      if (type->isIntegerTy()) {
        return builder_.CreateSDiv(L, R, "divtmp");
      } else if (type->isDoubleTy()) {
        return builder_.CreateFDiv(L, R, "divtmp");
      } else {
        break;
      }
    }
    
    case '<': {
      // '<='
      if (binOpExp->isEqualityLTRType()) {
        if (type->isIntegerTy()) {
          return builder_.CreateICmpSLE(L, R, "letmp"); // -> Int 0|1
        } else if (type->isDoubleTy()) {
          // FCMP_OGE -- True if LHS is ordered (not NAN) and less than or equal to RHS
          //
          //   (3.1 <= 3.4) -> 1
          //   (3.4 <= 3.4) -> 1
          //   (8.0 <= 3.4) -> 0
          //   (NAN <= 3.4) -> 0
          //   (NAN <= NAN) -> 0
          //   (9.0 <= NAN) -> 0
          //
          return builder_.CreateFCmpOLE(L, R, "letmp"); // -> Int 0|1
        }

      // '<'
      } else {
        if (type->isIntegerTy()) {
          return builder_.CreateICmpSLT(L, R, "lttmp"); // -> Int 0|1
        } else if (type->isDoubleTy()) {
          // True if LHS is ordered (not NAN) and less than RHS
          //
          //   (3.1 < 3.4) -> 1
          //   (3.4 < 3.4) -> 0
          //   (8.0 < 3.4) -> 1
          //   (NAN < 3.4) -> 0
          //   (NAN < NAN) -> 0
          //   (9.0 < NAN) -> 0
          //
          return builder_.CreateFCmpOLT(L, R, "lttmp"); // -> Int 0|1
        }
      }
      return error(R_FMT("Invalid operand type for binary operator \"" << binOpExp->operatorName() << "\"" ));
    }
    
    case '>': {
      // '>='
      if (binOpExp->isEqualityLTRType()) {
        if (type->isIntegerTy()) {
          return builder_.CreateICmpSGE(L, R, "getmp"); // -> Int 0|1
        } else if (type->isDoubleTy()) {
          // FCMP_OGE -- True if LHS is ordered (not NAN) and greater than or equal to RHS
          //
          //   (3.1 >= 3.4) -> 0
          //   (3.4 >= 3.4) -> 1
          //   (8.0 >= 3.4) -> 1
          //   (NAN >= 3.4) -> 0
          //   (NAN >= NAN) -> 0
          //   (9.0 >= NAN) -> 0
          //
          return builder_.CreateFCmpOGE(L, R, "getmp"); // -> Int 0|1
        }

      // '>'
      } else {
        if (type->isIntegerTy()) {
          return builder_.CreateICmpSGT(L, R, "gttmp"); // -> Int 0|1
        } else if (type->isDoubleTy()) {
          // FCMP_OGT -- True if LHS is ordered (not NAN) AND greater than RHS
          //
          //   (3.1 > 3.4) -> 0
          //   (3.4 > 3.4) -> 0
          //   (8.0 > 3.4) -> 1
          //   (NAN > 3.4) -> 0
          //   (NAN > NAN) -> 0
          //   (9.0 > NAN) -> 0
          //
          return builder_.CreateFCmpOGT(L, R, "gttmp"); // -> Int 0|1
        }
      }
      return error(R_FMT("Invalid operand type for binary operator \"" << binOpExp->operatorName() << "\"" ));
    }
    
    case '!': {
      // '!='
      if (binOpExp->isEqualityLTRType()) {
        if (type->isIntegerTy()) {
          return builder_.CreateICmpNE(L, R, "netmp"); // -> Int 0|1
        } else if (type->isDoubleTy()) {
          // We use UNE, since the ONE behavior ((NAN != 3.4) -> False) is a weirder than
          // ((NAN != NAN) -> True).
          //
          // FCMP_UNE -- True if (LHS is?) unordered (NAN) OR operands are unequal
          //
          //   (3.1 != 3.4) -> 1
          //   (3.4 != 3.4) -> 0
          //   (NAN != 3.4) -> 1
          //   (NAN != NAN) -> 1
          //   (3.0 != 3.4) -> 0
          //
          return builder_.CreateFCmpUNE(L, R, "netmp"); // -> Int 0|1
        }
        return error(R_FMT("Invalid operand type for binary operator \"" << binOpExp->operatorName() << "\"" ));
      }
      // Note: There's no "else" since there's no '!' infix operator
      break;
    }
    
    case '=': {
      // '=='
      if (binOpExp->isEqualityLTRType()) {
        if (type->isIntegerTy()) {
          return builder_.CreateICmpEQ(L, R, "eqtmp"); // -> Int 0|1
        } else if (type->isDoubleTy()) {
          // True if (LHS is?) ordered (not NAN) and equal
          //
          //      FCMP_OEQ (==)          FCMP_UNE (!=)
          //   (3.1 == 3.4) -> 0      (3.1 != 3.4) -> 1
          //   (3.4 == 3.4) -> 1      (3.4 != 3.4) -> 0
          //   (NAN == 3.4) -> 0      (NAN != 3.4) -> 1
          //   (NAN == NAN) -> 0      (NAN != NAN) -> 1
          //   (3.0 == 3.4) -> 1      (3.0 != 3.4) -> 0
          //
          return builder_.CreateFCmpOEQ(L, R, "eqtmp"); // -> Int 0|1
        }
        return error(R_FMT("Invalid operand type for binary operator \"" << binOpExp->operatorName() << "\"" ));
      }
      // Note: There's no "else" since '=' should never exist as a binop, as it
      //       has a special class ast::Assignment.
      break;
    }
    
    default: break;
  }
  
  return error(R_FMT("Invalid binary operator \"" << binOpExp->operatorName() << "\"" ));
}

#include "_VisitorImplFooter.h"
