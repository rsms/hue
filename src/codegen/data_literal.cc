#include "_VisitorImplHeader.h"


Value *Visitor::codegenDataLiteral(const ast::DataLiteral *dataLit) {
  DEBUG_TRACE_LLVM_VISITOR;
  
  //return wrap(ConstantArray::get(*unwrap(C), StringRef(Str, Length), DontNullTerminate == 0));
  //bool AddNull = false;
  //ConstantArray::get(getGlobalContext(), StringRef(Str, Length), AddNull);
  // ByteString::const_iterator it = dataLit->data().begin();
  // ByteString::const_iterator it_end = dataLit->data().end();
  // size_t i = 0, count = dataLit->data().size();
  // std::vector<Constant*> elems;
  // elems.reserve(count);
  // 
  // for (; it != it_end; ++it, ++i) {
  //   elems.push_back( ConstantInt::get(getGlobalContext(), APInt(8, *it, false)) );
  // }
  //ArrayType::get(Type::FloatTy, 4);
  //ArrayRef<Constant*> V = makeArrayRef(elems);
  
  // Create constant array with the actual bytes
  const bool addNull = false;
  Constant* arrayV = ConstantArray::get(getGlobalContext(),
      StringRef(reinterpret_cast<const char*>(dataLit->data().data()), dataLit->data().size()),
      addNull);
  
  GlobalVariable* arrayGV = createArray(arrayV, "data");
  
  Type* T = PointerType::get(getArrayStructType(builder_.getInt8Ty()), 0);
  return builder_.CreatePointerCast(arrayGV, T);
  
  // wrapConstantArrayValueInGEP:
  // Value *zero = ConstantInt::get(Type::getInt32Ty(getGlobalContext()), 0);
  // Value *Args[] = { zero, zero };
  // //Value* V = builder_.CreateInBoundsGEP(gV, Args, "dataptr");
  // Value* V = builder_.CreateGEP(gV, Args, "dataptr");
}

#include "_VisitorImplFooter.h"
