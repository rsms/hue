// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.
#include "Mangle.h"

#include <string>
#include <sstream>
#include <assert.h>

namespace hue {

// MangledTypeList = MangledType*
std::string mangle(const ast::TypeList& types) {
  std::ostringstream ss;
  ast::TypeList::const_iterator it = types.begin();
  for (; it != types.end(); ++it) {
    ast::Type* T = *it;
    ss << mangle(*T);
  }
  return ss.str();
}

// MangledVarList = MangledTypeList
std::string mangle(const ast::VariableList& vars) {
  std::ostringstream ss;
  ast::VariableList::const_iterator it = vars.begin();
  for (; it != vars.end(); ++it) {
    assert((*it)->type() != 0);
    ss << mangle(*(*it)->type());
  }
  return ss.str();
}

// MangledFuncType = '$' MangledVarList '$' MangledTypeList
//
// (a Int) Int -> $I$I
//
std::string mangle(const ast::FunctionType& funcType) {
  std::string mname = "$";
  ast::VariableList* args = funcType.args();
  if (args) mname += mangle(*args);
  mname += '$';
  ast::Type* returnType = funcType.returnType();
  if (returnType) mname += mangle(*returnType);
  return mname;
}

/*
Itanium C++ ABI name mangling rules:

<builtin-type>
   ::= v  # void
   ::= w  # wchar_t
   ::= b  # bool
   ::= c  # char
   ::= a  # signed char
   ::= h  # unsigned char
   ::= s  # short
   ::= t  # unsigned short
   ::= i  # int
   ::= j  # unsigned int
   ::= l  # long
   ::= m  # unsigned long
   ::= x  # long long, __int64
   ::= y  # unsigned long long, __int64
   ::= n  # __int128
   ::= o  # unsigned __int128
   ::= f  # float
   ::= d  # double
   ::= e  # long double, __float80
   ::= g  # __float128
   ::= z  # ellipsis
               ::= Dd # IEEE 754r decimal floating point (64 bits)
               ::= De # IEEE 754r decimal floating point (128 bits)
               ::= Df # IEEE 754r decimal floating point (32 bits)
               ::= Dh # IEEE 754r half-precision floating point (16 bits)
               ::= Di # char32_t
               ::= Ds # char16_t
               ::= Da # auto (in dependent new-expressions)
               ::= Dn # std::nullptr_t (i.e., decltype(nullptr))
   ::= u <source-name>  # vendor extended type
*/

// MangledType = <ASCII string>
std::string mangle(const hue::ast::Type& T) {
  if (T.typeID() == ast::Type::Named) {
    std::string utf8name = T.name().UTF8String();
    char buf[12];
    int len = snprintf(buf, 12, "N%zu", utf8name.length());
    utf8name.insert(0, buf, len);
    return utf8name;
  } else switch (T.typeID()) {
    case ast::Type::Float: return "d";
    case ast::Type::Int:   return "x";
    case ast::Type::Char:  return "j";
    case ast::Type::Byte:  return "a";
    case ast::Type::Bool:  return "b";
    case ast::Type::Func:  return "F"; // TODO
    default:    return "";
  }
}

ast::Type demangle(const std::string& mangleID) {
  switch (mangleID[0]) {
    case 'd': return ast::Type(ast::Type::Float);
    case 'x': return ast::Type(ast::Type::Int);
    case 'j': return ast::Type(ast::Type::Char);
    case 'a': return ast::Type(ast::Type::Byte);
    case 'b': return ast::Type(ast::Type::Bool);
    case 'F': return ast::Type(ast::Type::Func);
    case 'N': {
      return ast::Type(Text(mangleID.substr(1)));
    }
    default: return ast::Type(ast::Type::Unknown);
  }
}

std::string mangle(llvm::Type* T) {
  switch(T->getTypeID()) {
    case llvm::Type::VoidTyID:      return "v";    ///<  0: type with no size
    case llvm::Type::FloatTyID:     return "f";        ///<  1: 32-bit floating point type
    case llvm::Type::DoubleTyID:    return "d";      ///<  2: 64-bit floating point type
    case llvm::Type::X86_FP80TyID:  return "e";    ///<  3: 80-bit floating point type (X87)
    case llvm::Type::FP128TyID:     return "g";       ///<  4: 128-bit floating point type (112-bit mantissa)
    case llvm::Type::PPC_FP128TyID: return "De";   ///<  5: 128-bit floating point type (two 64-bits, PowerPC)
    //case llvm::Type::LabelTyID:     return        ///<  6: Labels
    //case llvm::Type::MetadataTyID:  return     ///<  7: Metadata
    //case llvm::Type::X86_MMXTyID:   return      ///<  8: MMX vectors (64 bits, X86 specific)

    // Derived types... see DerivedTypes.h file.
    // Make sure FirstDerivedTyID stays up to date!
    case llvm::Type::IntegerTyID: {
      switch (T->getPrimitiveSizeInBits()) {
        case 8: return "a";
        case 16: return "t";
        case 32: return "j";
        case 64: return "x";
        default: return "?";
      }
    }
    case llvm::Type::FunctionTyID:
      return std::string("F") + mangle(static_cast<llvm::FunctionType*>(T));    ///< 10: Functions
    
    // case StructTyID: return       ///< 11: Structures
    // case ArrayTyID: return        ///< 12: Arrays
    // case PointerTyID: return      ///< 13: Pointers
    // case VectorTyID: return       ///< 14: SIMD 'packed' format, or other vector type
    default: return "?";
  }
}


std::string mangle(llvm::FunctionType* FT) {
  std::string mname = "$";
  for (llvm::FunctionType::param_iterator I = FT->param_begin(),
       E = FT->param_end();
       I != E; ++I)
  {
    mname += mangle(*I);
  }
  mname += '$';
  mname += mangle(FT->getReturnType());
  return mname;
}


} // namespace hue
