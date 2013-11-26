#ifndef LLVM_TRANSFORMS_POINTERSSI_POINTERSSI_H_
#define LLVM_TRANSFORMS_POINTERSSI_POINTERSSI_H_

#include "AliasSets.h"

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include <set>
#include <map>

// TODO:
// Use AliasSetTracker to rename pointer aliases.

namespace llvm {

class PointerRedefinition : public ModulePass {

public:
  static char ID;
  PointerRedefinition() : ModulePass(ID),
    StatNumCreatedSigmas_(0), StatNumCreatedPhis_(0), StatNumInstructions_(0) { }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);
  bool doFinalization(Module& M); 

  static StringRef GetRedefPrefix() { return "ptr_redef"; }
  static StringRef GetPhiPrefix()   { return "ptr_phi"; }

private:
  void createRedefsInFunction(Function *F);
  void createRedefsForInst(Instruction *I);
  void createRedefsForGEP(GetElementPtrInst *GEP); 
  void createRedefsForCallInst(CallInst *CI); 
  void createPhiNodeForRedef(Instruction *I); 
  PHINode *createPhiNodeAt(Value *V, BasicBlock *BB); 

  bool dominatesUse(Value *V, BasicBlock *BB); 
  bool dominatesUse(Value *V, Value *U); 
  bool dominates(Value *Def, Value *V); 
  void replaceUsesOfWithAfter(Value *V, Value *R, BasicBlock *BB); 
  void replaceUsesOfWithAfter(Value *V, Value *R, Instruction *I); 

  void constfn() const;

  Function *F_;

  AliasSets *AS_;
  CallGraph *CG_;
  DominatorTree *DT_;
  DominanceFrontier *DF_;
  TargetLibraryInfo *TLI_;

  DenseMap<Function*, std::set<Value*>> StateChangers_; 

  // Statistics.
  unsigned StatNumCreatedSigmas_;
  unsigned StatNumCreatedPhis_;
  unsigned StatNumInstructions_;
};

}

#endif

