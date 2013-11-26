#include "AliasSets.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;

static cl::opt<bool> AliasSetsDebug("alias-sets-debug",
                                    cl::desc("Enable debugging"),
                                    cl::Hidden, cl::init(false));

static cl::opt<std::string>
  AliasSetsDebugFunction("alias-sets-debug-function",
                         cl::desc("Enable debugging"),
                         cl::Hidden, cl::init(""));

#define AS_DEBUG(X) { if (AliasSetsDebug) { X; } }

// getAnalysisUsage
void AliasSets::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AliasAnalysis>();
  AU.setPreservesAll();
}

// runOnFunction
bool AliasSets::runOnFunction(Function &F) {
  //if (AliasSetsDebugFunction == "__all__") {
  //  dbgs() << F.getName() << "\n";
  //} else if (!AliasSetsDebugFunction.empty()) {
  //  if (F.getName() != AliasSetsDebugFunction)
  //    return true;
  //  errs() << "DEBUGGING FUNCTION\n";
  //  dbgs() << F;
  //}

  //errs() << "(AliasSets) Function: " << F.getName() << "\n";

  Ptrs_.clear();
  Sets_.clear();

  AA_ = &getAnalysis<AliasAnalysis>();

  for (auto AI = F.arg_begin(), AE = F.arg_end(); AI != AE; ++AI)
    visitArgument(&(*AI));

  for (auto& BB : F)
    for (auto& I : BB)
      visitInstruction(&I);

  /* AS_DEBUG(
    std::vector<Value*> Leftovers;
    for (auto& P : Ptrs_)
      Leftovers.push_back(P);
    for (auto& P : Leftovers)
      createAndRegisterAliasSet(P);
    print(dbgs())
  ); */

  return false;
}

void AliasSets::print(raw_ostream& OS) const {
  for (auto& P : Sets_) {
    OS << "## " << *P.first << "\n";
    for (auto& T : *P.second)
      OS << "   " << *T << "\n";
  }
}

void AliasSets::visitArgument(Argument *A) {
  if (A->getType()->isPointerTy()) {
    //errs() << "Inserting: " << *A << "\n";
    Ptrs_.insert(A);
  }
}

void AliasSets::visitInstruction(Instruction *I) {
  //errs() << "Visiting instruction: " << *I << "\n";
  if (I->getType()->isPointerTy()) {
    //errs() << "Inserting: " << *I << "\n";
    Ptrs_.insert(I);
  }
}

std::set<Value*> *AliasSets::getAliasSet(Value *V) {
  assert(V->getType()->isPointerTy() && "Value must be a pointer");
  assert(!isa<GlobalValue>(V) && !isa<Constant>(V) &&
         "Value must not be a global or a constant");

  auto It = Sets_.find(V);
  if (It != Sets_.end())
    return It->second;

  return createAndRegisterAliasSet(V);
}

std::set<Value*> *AliasSets::getOrAllocateAliasSet(Value *V) {
  assert(V->getType()->isPointerTy() && "Value must be a pointer");

  auto It = Sets_.find(V);
  if (It != Sets_.end())
    return It->second;

  std::set<Value*> *Set = new std::set<Value*>();
  Sets_.insert(std::make_pair(V, Set));
  return Set;
}

std::set<Value*> *AliasSets::getAliasSetOrNull(Value *V) {
  assert(V->getType()->isPointerTy() && "Value must be a pointer");

  auto It = Sets_.find(V);
  if (It != Sets_.end())
    return It->second;

  return NULL;
}

void AliasSets::bind(Value *V, Value *Parent) {
  AS_DEBUG(dbgs() << "(AliasSets) bind: " << *V << ", " << *Parent << "\n");
  //errs() << "bind: " << *V << ", " << *Parent << "\n";
  assert(!Ptrs_.count(V));
  if (Ptrs_.count(Parent))
    createAndRegisterAliasSet(Parent);
  std::set<Value*> *Set = Sets_[Parent];
  Set->insert(V);
  Sets_[V] = Set;
}

std::set<Value*> *AliasSets::createAndRegisterAliasSet(Value *V) {
  //errs() << "createAndRegisterAliasSet: " << *V << "\n";
  if (!Ptrs_.count(V))
    errs() << "## V = " << *V << ", " << Sets_.count(V) << "\n";
  assert(Ptrs_.count(V) && "Value not in pointer set");

  std::vector<Value*> MustAlias;
  std::set<Value*> *Set = getOrAllocateAliasSet(V);

  Ptrs_.erase(V);
  Set->insert(V);
  for (auto& P : Ptrs_) {
    AliasAnalysis::AliasResult Result = AA_->alias(P, V);
    if (Result == AliasAnalysis::MustAlias) {
      MustAlias.push_back(P);
    } else if (Result != AliasAnalysis::NoAlias) {
      std::set<Value*> *PSet = getOrAllocateAliasSet(P);
      PSet->insert(V);
      Set->insert(P);
    }
  }

  Sets_.insert(std::make_pair(V, Set));

  for (auto& P : MustAlias) {
    std::set<Value*> *PSet = getAliasSetOrNull(P);
    if (PSet)
      delete PSet;
    //errs() << "Alias: " << *P << "\n";
    Set->insert(P);
    Sets_[P] = Set;
    Ptrs_.erase(P);
  }

  return Set;
}

char AliasSets::ID = 0;
static RegisterPass<AliasSets>
  X("alias-sets", "Pointer redefinition for sparse abstract interperatation");

