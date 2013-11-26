#ifndef _ALIASSETS_H_
#define _ALIASSETS_H_

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

#include <set>
#include <map>

// TODO:
// Use AliasSetTracker to rename pointer aliases.

namespace llvm {

class AliasSets : public FunctionPass {

public:
  static char ID;
  AliasSets() : FunctionPass(ID) { }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnFunction(Function &F); 
  virtual void print(raw_ostream& OS) const; 

  void bind(Value *V, Value *Parent);

  std::set<Value*> *getAliasSet(Value *V);

private:
  void visitInstruction(Instruction *I);
  void visitArgument(Argument *I);


  std::set<Value*> *getOrAllocateAliasSet(Value *V);
  std::set<Value*> *getAliasSetOrNull(Value *V);

  std::set<Value*> *createAndRegisterAliasSet(Value *V); 

  AliasAnalysis *AA_;
  DominatorTree *DT_;

  std::set<Value*> Ptrs_;
  DenseMap<Value*, std::set<Value*>*> Sets_;
};

}

#endif

