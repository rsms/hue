#include "_VisitorImplHeader.h"

Value* Visitor::castValueToBool(Value* V) {
  Type* T = V->getType();
  if (T->isIntegerTy()) {
    // <Int> -> (<Int> != 0) -> <Bool>
    //Value* i64_0 = ConstantInt::get(llvm::getGlobalContext(), APInt(64, 0, true));
    //return builder_.CreateICmpNE(testV, i64_0, "eqtmp");
    
    // <Int> -> cast(<Int>, <Bool>) -> <Bool>
    return builder_.CreateIntCast(V, builder_.getInt1Ty(), true, "casttmp");

  } else if (T->isDoubleTy()) {
    // <Float> -> (<Float> != 0.0) -> <Bool>
    Value* double_0 = ConstantFP::get(getGlobalContext(), APFloat(0.0));
    // FCMP_ONE -- True if ordered AND operands are unequal
    return builder_.CreateFCmpONE(V, double_0, "eqtmp");

  } else if (T->isPointerTy()) {
    // <*> -> cast(<*>, <Bool>) -> <Bool>
    return builder_.CreatePtrToInt(V, builder_.getInt1Ty(), "casttmp");
  } else {
    return error("Unable to cast value into boolean");
  }
}

Type* Visitor::highestFidelityType(Type* T1, Type* T2) {
  if (T1 == T2) {
    return T1;
  } else if (T1->isIntegerTy() && T2->isDoubleTy()) {
    return T2;
  } else if (T2->isIntegerTy() && T1->isDoubleTy()) {
    return T1;
  } else {
    // Unknown
    return 0;
  }
}

Value* Visitor::castValueTo(Value* inputV, Type* destT) {
  Type* inputT = inputV->getType();
  
  // ?->Bool:
  if (destT->isIntegerTy(1))
    return castValueToBool(inputV);
  
  // Different types?
  if (inputT->getTypeID() != destT->getTypeID()) {
    
    // Int->Float:
    if (inputT->isIntegerTy() && destT->isDoubleTy()) {
      return builder_.CreateSIToFP(inputV, destT, "casttmp");
    
    // Float->Int: Error
    } else if (destT->isIntegerTy() && inputT->isDoubleTy()) {
      // warning("Implicit conversion of floating point value to integer storage");
      // V = builder_.CreateFPToSI(V, destT, "casttmp");
      return error("Illegal conversion from floating point number to integer");
    }
    
    return 0;
  
  // Int->Int: iX to iY
  } else if (destT->isIntegerTy() && destT->getPrimitiveSizeInBits() != inputT->getPrimitiveSizeInBits()) {
    if (   destT->getPrimitiveSizeInBits() < inputT->getPrimitiveSizeInBits()
        && !inputT->canLosslesslyBitCastTo(destT)) {
      warning("Implicit truncation of integer");
    }
    
    // This helper function will make either a bit cast, int extension or int truncation
    return builder_.CreateIntCast(inputV, destT, /*isSigned = */true);

  // Float->Float: fpX to fpY
  } else if (destT->isDoubleTy() && destT->getPrimitiveSizeInBits() != inputT->getPrimitiveSizeInBits()) {
    if (inputT->canLosslesslyBitCastTo(destT)) {
      return builder_.CreateBitCast(inputV, destT, "bcasttmp");
    } else if (destT->getPrimitiveSizeInBits() < inputT->getPrimitiveSizeInBits()) {
      warning("Implicit truncation of floating point number");
      return builder_.CreateFPTrunc(inputV, destT, "fptrunctmp");
    } else {
      return builder_.CreateFPExt(inputV, destT, "fpexttmp");
    }
  }
  
  return inputV;
}

#include "_VisitorImplFooter.h"
