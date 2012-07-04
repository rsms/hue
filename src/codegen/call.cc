// Copyright (c) 2012, Rasmus Andersson. All rights reserved. Use of this source
// code is governed by a MIT-style license that can be found in the LICENSE file.

#include "_VisitorImplHeader.h"

// Returns true if V can be used as the target in a call instruction
inline static bool valueIsCallable(Value* V) {
  return !!Visitor::functionTypeForValue(V);
}

std::string Visitor::formatFunctionCandidateErrorMessage(const ast::Call* node,
                                                         const FunctionSymbolTargetList& candidateFuncs,
                                                         CandidateError error) const
{
  std::string utf8calleeName = node->symbol()->toString();
  std::ostringstream ss;
  
  if (error == CandidateErrorArgCount) {
    ss << "No function matching call to ";
  } else if (error == CandidateErrorArgTypes) {
    ss << "No function with arguments matching call to ";
  } else if (error == CandidateErrorReturnType) {
    ss << "No function with result matching call to ";
  } else if (error == CandidateErrorAmbiguous) {
    ss << "Ambiguous function call to ";
  } else {
    ss << "No viable function for call to ";
  }
  
  // TODO: Proper toHueSource() repr here:
  ss << utf8calleeName << "/" << node->arguments().size() << ". ";
  
  if (error == CandidateErrorArgCount) {
    ss << "Did you mean to call any of these functions?";
  } else if (error == CandidateErrorReturnType) {
    ss << "Express what type of result you expect. Available functions:";
  } else {
    ss << "Candidates are:";
  }
  
  ss << std::endl;
  FunctionSymbolTargetList::const_iterator it2 = candidateFuncs.begin();
  for (; it2 != candidateFuncs.end(); ++it2) {
    ss << "  " << utf8calleeName << ' ' << (*it2).hueType->toHueSource() << std::endl;
  }
  
  return ss.str();
}


Value *Visitor::codegenCall(const ast::Call* node, const ast::Type* expectedReturnType/* = 0*/) {
  DEBUG_TRACE_LLVM_VISITOR;
  const ast::Symbol& symbol = *node->symbol();

  // TODO FIXME: This will fail for lookups into any structs (e.g. foo:bar) since
  //             we have not implemented struct traversal for symbol lookup, in the
  //             special function symbol table.
  if (symbol.isPath()) {
    return error(std::string("Not implemented: Struct traversal for function symbols (at symbol \"")
                 + symbol.toString() + "\")");
  }
  
  // Look up a list of matching function symbols
  FunctionSymbolTargetList candidateFuncs = lookupFunctionSymbols(symbol);
  
  // No symbol?
  if (candidateFuncs.empty()) {
    // TODO: We could take all known symbols, order them by edit distance and suggest
    //       the top N (N is perhaps controlled by a quality threshold) symbol names.
    return error(std::string("Unknown symbol \"") + node->symbol()->toString() + "\"");
  }
  
  // Local ref to input arguments, for our convenience.
  const ast::Call::ArgumentList& arguments = node->arguments();
  
  // Filter out functions that take a different number of arguments
  FunctionSymbolTargetList::const_iterator it = candidateFuncs.begin();
  FunctionSymbolTargetList candidateFuncsMatchingArgCount;
  for (; it != candidateFuncs.end(); ++it) {
    if (static_cast<size_t>((*it).type->getNumParams()) == arguments.size())
      candidateFuncsMatchingArgCount.push_back(*it);
  }
  
  // No candidates with the same number of arguments?
  if (candidateFuncsMatchingArgCount.empty()) {
    return error(formatFunctionCandidateErrorMessage(node, candidateFuncs, CandidateErrorArgCount));
  }
  // TODO: If candidateFuncs.size() == 1 here, we can take some shortcuts, perhaps.
  
  // Now, we pause the function candidate selection for a little while and
  // visit the arguments. We need to do this in order to know the types of
  // the arguments. The types need to be resolved in order for us to be able
  // to find a matching function candidate.

  // Build argument list by codegen'ing all input variables
  std::vector<Value*> argValues;
  ast::Call::ArgumentList::const_iterator inIt = arguments.begin();
  for (; inIt != arguments.end(); ++inIt) {
    // Codegen the input value and store it
    Value* inputV = codegen(*inIt);
    if (inputV == 0) return 0;
    argValues.push_back(inputV);
  }
  
  // Find a function that matches the arguments
  FunctionSymbolTargetList::const_iterator it2 = candidateFuncsMatchingArgCount.begin();
  FunctionSymbolTargetList candidateFuncsMatchingTypes;
  for (; it2 != candidateFuncsMatchingArgCount.end(); ++it2) {
    FunctionType* candidateFT = (*it2).type;
    size_t i = 0;
    FunctionType::param_iterator ftPT = candidateFT->param_begin();
    bool allTypesMatch = true;
    assert(argValues.size() == candidateFT->getNumParams());
    
    // Check each argument type
    for (; ftPT != candidateFT->param_end(); ++ftPT, ++i) {
      Type* argT = argValues[i]->getType();
      Type* expectT = *ftPT;
      
      // Unless the types are equal, remove the candidate function from the list and
      // stop checking types.
      if (argT != expectT) { // a better cmp, e.g. "isCompatible"
        allTypesMatch = false;
        break;
      }
    }
    
    // If all argument types matched, this is a viable candidate
    if (allTypesMatch)
      candidateFuncsMatchingTypes.push_back(*it2);
  }
  
  // No candidates?
  if (candidateFuncsMatchingTypes.empty()) {
    return error(formatFunctionCandidateErrorMessage(node, candidateFuncsMatchingArgCount,
        CandidateErrorArgTypes));
  }
  
  
  // We will later store the selected candidate here
  Value* targetV = 0;
  FunctionType* targetFT = 0;

  
  // If we have some expected return types, try to use that information to narrow down our
  // list of candidates.
  if (expectedReturnType != 0) {
    
    // Reduce our list of candidates according to expected return types
    FunctionSymbolTargetList candidateFuncsMatchingReturns;
    FunctionSymbolTargetList::const_iterator it3 = candidateFuncsMatchingTypes.begin();
    for (; it3 != candidateFuncsMatchingTypes.end(); ++it3) {

      // Get the list types that the candidate returns
      const ast::Type* candidateReturnType = (*it3).hueType->resultType();
      
      // Number of results must match, or we ignore this candidate
      if (!expectedReturnType->isEqual(*candidateReturnType)) {
        continue; // ignore candidate
      }
      
      if (expectedReturnType == 0) {
        // Expects no result and candidate does not return anything
        if (candidateReturnType == 0) {
          candidateFuncsMatchingReturns.push_back(*it3);
        }
      } else if (   expectedReturnType->typeID() == ast::Type::Unknown
                 && candidateFuncsMatchingTypes.size() == 1) {
        // Expected type should be inferred and we have only a single candidate.
        // This means that we chose this candidate and whatever whats its type inferred will
        // do so based on whatever this candidate returns.
        candidateFuncsMatchingReturns.push_back(*it3);

      } else if (candidateReturnType->isEqual(*expectedReturnType)) {
        // The return types are equal.
        candidateFuncsMatchingReturns.push_back(*it3);
      }
      
    }
    
    // Did we find a single candidate?
    if (candidateFuncsMatchingReturns.size() == 1) {
      targetV = candidateFuncsMatchingReturns[0].value;
      targetFT = candidateFuncsMatchingReturns[0].type;
    } else if (candidateFuncsMatchingReturns.size() == 0) {
      return error(formatFunctionCandidateErrorMessage(node, candidateFuncsMatchingTypes,
          CandidateErrorReturnType));
    } else {
      return error(formatFunctionCandidateErrorMessage(node, candidateFuncsMatchingTypes,
          CandidateErrorAmbiguous));
    }
    
  } else {
    // We don't have any hints about expected return values to go on.
    // If the have a single candidate, let's use that. Otherwise it's an error.
    if (candidateFuncsMatchingTypes.size() == 1) {
      targetV = candidateFuncsMatchingTypes[0].value;
      targetFT = candidateFuncsMatchingTypes[0].type;
    } else {
      return error(formatFunctionCandidateErrorMessage(node, candidateFuncsMatchingTypes,
          CandidateErrorAmbiguous));
    }
  }
  
  // Create call instruction
  if (targetFT->getReturnType()->isVoidTy()) {
    builder_.CreateCall(targetV, argValues);
    return ConstantInt::getFalse(getGlobalContext());
  } else {
    return builder_.CreateCall(targetV, argValues, node->symbol()->toString() + "_res");
  }
}

#include "_VisitorImplFooter.h"
