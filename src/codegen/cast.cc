// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
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


static const int64_t FloatMaxFiniteValue = 9218868437227405312;
static const int64_t FloatMinFiniteValue = -9218868437227405312;

Value* Visitor::castValueTo(Value* inputV, Type* destT) {
  Type* inputT = inputV->getType();
  
  // ?->Bool:
  if (destT->isIntegerTy(1))
    return castValueToBool(inputV);
  
  // Different types?
  if (inputT != destT) {
    
    // Int->Float:
    if (inputT->isIntegerTy() && destT->isDoubleTy()) {

      // Is the integer larger than the 1:1 numbers able to be represented by a
      // double (limited by the mantissa)?
      if (ConstantInt::classof(inputV)) {
        ConstantInt* intV = (ConstantInt*)inputV;
        const APInt& apint = intV->getValue();
        //int64_t v = (int64_t)intV->getLimitedValue();
        if (   (intV->isNegative() && apint.slt((uint64_t)FloatMinFiniteValue))
            || (!intV->isNegative() && apint.sgt((uint64_t)FloatMaxFiniteValue)) ) {
        //if (   (intV->isNegative() && v < FloatMinFiniteValue)
        //    || (!intV->isNegative() && v > FloatMinFiniteValue) ) {
          warning("Implicit conversion from integer to floating-point number loses precision");
        }
      }

      return builder_.CreateSIToFP(inputV, destT, "casttmp");
    
    // Float->Int: Error
    } else if (destT->isIntegerTy() && inputT->isDoubleTy()) {
      // warning("Implicit conversion of floating point value to integer storage");
      // V = builder_.CreateFPToSI(V, destT, "casttmp");
      return error("Implicit conversion from floating-point number to integer");

    } else if (ArrayType::classof(destT) && ArrayType::classof(inputT)) {
      // Both sides are Array types. Let's check what elements they contain.
      Type* destElementT = static_cast<ArrayType*>(destT)->getElementType();
      Type* inputElementT = static_cast<ArrayType*>(inputT)->getElementType();
      if (destElementT != inputElementT) {
        return error("Different types of arrays");
      } else {
        // They are arrays of same kind, but different size. Since we do not have
        // the notion of explicit array size in type declarations, we can safely assume
        // that the destionation type is variable size, thus return a pointer.
        //return builder_.CreateGEP(Value *Ptr, ArrayRef<Value *> IdxList, "aptrtmp");
        //return builder_.CreateLoad(inputV, "aptrtmp");
        //Value *gv = CreateGlobalString(Str, Name);
        rlog("GEP 1");
        Value *zero = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0);
        rlog("GEP 2");
        Value *Args[] = { zero, zero };
        
        //Value* v = builder_.CreateInBoundsGEP(inputV, Args, "aptrtmp");
        Value* v = builder_.CreateGEP(inputV, Args, "aptrtmp");
        rlog("GEP -> " << v);
        return v;
        //return inputV;
      }

    } else if (inputT->isPointerTy()) {
      
      // Special case for constant arrays (essentially sequence literals, like data and text)
      ArrayType* arrayT = getArrayTypeFromPointerType(inputT);
      
      if (arrayT) {
        if (destT->isPointerTy() && destT->getNumContainedTypes() == 1) {
          //if (arrayT->isValidElementType(destT->getContainedType(0))) {
          if (arrayT->getElementType() == destT->getContainedType(0)) {
            rlog("inputT is a constant array of the same kind as expected by the destination");
            // Wrap in a GEP, so destination gets a pointer to the same type
            return wrapValueInGEP(inputV);
          } else {
            return error("Source array has different element type than destination array");
          }
        } else {
          return error("Source is an array but destination expects something else");
        }
      }
      
      // TODO...
      //rlog("inputT is a pointer. contained count: " << inputT->getNumContainedTypes()
      //  << ", containedType[0] = "); inputT->getContainedType(0)->dump();
      if (destT->isPointerTy()) {
        //rlog("destT is a pointer. contained count: " << destT->getNumContainedTypes()
        //     << ", containedType[0] = "); destT->getContainedType(0)->dump();
        if (inputT->getNumContainedTypes() == 1 && destT->getNumContainedTypes() == 1) {
          Type* inputCT = inputT->getContainedType(0);
          Type* destCT = destT->getContainedType(0);
          
          if (inputCT == destCT) {
            //rlog("pointers' contained types are the same");
            return inputV;
          } else if (static_cast<PointerType*>(destT)->isValidElementType(inputT)) {
            //rlog("inputCT: "); inputCT->dump();
            //rlog("destCT: "); destCT->dump();
            
            if (inputCT->isStructTy() && destCT->isStructTy()) {
              
              StructType* inputST = static_cast<StructType*>(inputCT);
              StructType* destST = static_cast<StructType*>(destCT);
              
              if (inputST->getNumElements() == destST->getNumElements()) {
                unsigned N = inputST->getNumElements();
                while (N--) {
                  Type* inputCT1 = inputST->getElementType(N);
                  Type* destCT1 = destST->getElementType(N);
                  if (inputCT1 != destCT1) {
                    if (!ArrayType::classof(inputCT1)) {
                      return error("Incompatible struct");
                    }
                    if (   destCT1->isPointerTy() && destCT1->getNumContainedTypes() == 1
                        && static_cast<llvm::ArrayType*>(inputCT1)->getElementType() == destCT1->getContainedType(0)) {
                      continue;
                    } else {
                      return error("Incompatible element types in array inside struct");
                    }
                  }
                }
                rlog("input pointers' contained type is compatible with destionation pointers' type");
                return builder_.CreatePointerCast(inputV, destT);
              }
            }
            
            // structs have different types
          }
        }
      } else {
        rlog("destT is NOT a pointer");
      }
    }
    
    else if (inputT->isStructTy()) {
      rlog("TODO: input is data struct");
    }
    
    return error(std::string("Unable to cast ") + typeName(inputT) + " to " + typeName(destT));
  
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
      // TODO: Make this an error if truncation could cause loss of precision
      warning("Implicit truncation of floating point number");
      return builder_.CreateFPTrunc(inputV, destT, "fptrunctmp");
    } else {
      return builder_.CreateFPExt(inputV, destT, "fpexttmp");
    }
  }
  
  return inputV;
}

#include "_VisitorImplFooter.h"
