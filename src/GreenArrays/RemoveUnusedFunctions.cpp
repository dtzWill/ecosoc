//===------------------------ RegionAnalysis.cpp --------------------------===//
//===----------------------------------------------------------------------===//
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Target/TargetLibraryInfo.h"

#include "PointerRedefinition.h"
#include "Expr.h"
#include "SymbolicRangeAnalysis.h"
#include "RegionAnalysis.h"

#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

/* ************************************************************************** */
/* ************************************************************************** */

namespace llvm {

class RemoveUnusedFunctions : public ModulePass {
public:
  static char ID;
  RemoveUnusedFunctions() : ModulePass(ID) { }

  virtual bool runOnModule(Module &M);
};

}
  
using namespace llvm;
using std::set;

bool RemoveUnusedFunctions::runOnModule(Module& M) {
  set<Function*> Functions;
  unsigned NumRemoved = 0;

  for (auto& F : M) {
    if (F.getName() == "main")
      continue;
    // errs() << F.getName() << ", " << F.getNumUses() << "\n";
    if (!F.getNumUses())
      Functions.insert(&F);
  }

  for (auto& F : Functions) {
    NumRemoved++;
    // errs() << "Removing: " << F->getName() << "\n";
    F->eraseFromParent();
    // errs() << "Removed\n";
  }

  dbgs() << "==================== RUF ====================\n";
  dbgs() << "==== # of removed functions:   " << NumRemoved << "\n";

  return false;
}

char RemoveUnusedFunctions::ID = 0;
static RegisterPass<RemoveUnusedFunctions> X("remove-unused-functions", "Remove unused functions");
