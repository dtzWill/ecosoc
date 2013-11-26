#ifndef _REGIONANALYSIS_H_
#define _REGIONANALYSIS_H_

#include "Expr.h"
#include "Range.h"
#include "SymbolicRangeAnalysis.h"

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

namespace llvm {

using std::vector;
using std::pair;

class Pointer;
class RegionAnalysis;

raw_ostream& operator<<(raw_ostream& OS, const Pointer& J);
raw_ostream& operator<<(raw_ostream& OS, const RegionAnalysis& R);

/***********
 * Pointer *
 ***********/
class Pointer {
public:
  typedef vector<Pointer*> ArgsTy;

  Pointer(Value *Value);
  Pointer(Value *Value, Range R);

  // RTTI for Pointer.
  enum ValueId { BASE_PTR, CALL_PTR, NOOP_PTR, INDEX_PTR, PHI_PTR }; 
  virtual ValueId getValueId() const = 0;
  static bool classof(Pointer const *) { return true; }

  ArgsTy::iterator begin() { return Args_.begin(); }
  ArgsTy::iterator end()   { return Args_.end(); }

  ArgsTy::const_iterator begin() const { return Args_.begin(); }
  ArgsTy::const_iterator end()   const { return Args_.end(); }

  unsigned getIdx()   const { return Idx_; }
  Value   *getValue() const { return Value_; }

  virtual void printProlog(raw_ostream& OS) const = 0;
  virtual void printEpilog(raw_ostream& OS) const = 0;

  virtual void print(raw_ostream &OS)      const;
  virtual void printAsDot(raw_ostream &OS) const;

  virtual Range eval() = 0;

  void     incIterations()       { Iterations_++; }
  unsigned getIterations() const { return Iterations_; }

  void  setRange(Range R);
  Range getRange() const { return R_; }

  ArgsTy getArgs() const { return Args_; }

  void     setArg(unsigned Idx, Pointer *J);
  void     pushArg(Pointer *J);
  bool     hasArg(Pointer *J)   const;
  size_t   getNumArgs()         const;
  Pointer *getArg(unsigned Idx) const;

  void subs(DenseMap<Pointer*, Pointer*> Subs); 

private:
  // Juction node counts for printing to dot graphs.
  Value *Value_;

  Range R_;
  unsigned Iterations_;

  static unsigned Idxs_;
  unsigned Idx_;

  ArgsTy Args_;
};

/***************
 * BasePointer *
 ***************/
class BasePointer : public Pointer {
public:
  BasePointer(Value *V);
  BasePointer(Value *V, Range R);

  // RTTI for BasePointer.
  virtual ValueId getValueId() const { return BASE_PTR; }
  static bool classof(Pointer const *P) {
    return P->getValueId() == BASE_PTR;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "base"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Range eval();
};

/***************
 * CallPointer *
 ***************/
class CallPointer : public Pointer {
public:
  CallPointer(CallInst *CI, Range R, vector<pair<Range, Range> > Subs);

  // RTTI for CallPointer.
  virtual ValueId getValueId() const { return CALL_PTR;}
  static bool classof(Pointer const *P) {
    return P->getValueId() == CALL_PTR;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "call"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Range eval();

private:
  vector<pair<Range, Range> > Subs_;
};

/***************
 * NoopPointer *
 ***************/
class NoopPointer : public Pointer {
public:
  NoopPointer(Value *V);
  NoopPointer(Value *V, Pointer *Incoming);

  // RTTI for NoopPointer.
  virtual ValueId getValueId() const { return NOOP_PTR;}
  static bool classof(Pointer const *P) {
    return P->getValueId() == NOOP_PTR;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "noop"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Range eval();
  void setZero();
private:
  bool IsZero_;
};

/****************
 * IndexPointer *
 ****************/
class IndexPointer : public Pointer {
public:
  IndexPointer(GetElementPtrInst *GEP, Pointer *Incoming, Range Index);

  // RTTI for IndexPointer.
  virtual ValueId getValueId() const { return INDEX_PTR;}
  static bool classof(Pointer const *P) {
    return P->getValueId() == INDEX_PTR;
  }

  virtual void printProlog(raw_ostream& OS) const { OS << "index"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Range eval();

private:
  Range Index_;
};

/***************
 * PhiPointer *
 ***************/
class PhiPointer : public Pointer {
public:
  PhiPointer(Value *V);

  // RTTI for PhiPointer.
  virtual ValueId getValueId() const { return PHI_PTR;}
  static bool classof(Pointer const *P) {
    return P->getValueId() == PHI_PTR;
  }

  bool isWidened()  const { return Widened_; }
  void setWidened()       { Widened_ = true; }

  void pushIncoming(Pointer *Incoming);

  virtual void printProlog(raw_ostream& OS) const { OS << "phi"; }
  virtual void printEpilog(raw_ostream& OS) const { }

  virtual Range eval();

private:
  bool Widened_;
};

/******************
 * RegionAnalysis *
 ******************/
class RegionAnalysis : public ModulePass {
public:
  static char ID;
  RegionAnalysis() : ModulePass(ID) { }

  typedef DenseMap<Value*, Pointer*> PointersMapTy;

  PointersMapTy::iterator begin() { return PointersMap_.begin(); }
  PointersMapTy::iterator end()   { return PointersMap_.end(); }

  PointersMapTy::const_iterator begin() const { return PointersMap_.begin(); }
  PointersMapTy::const_iterator end()   const { return PointersMap_.end(); }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnModule(Module &M);

  virtual void print(raw_ostream& OS) const;
  void printAsDot(raw_ostream& OS) const;
  void printInfo(raw_ostream& OS) const;

  Range getRange(Value *V);
  Pointer *getPointerForValue(Value *V); 

private:
  void setPointer(Value *V, Pointer *J); 

  Range getRangeForFunction(Function *F);
  Range getRangeForCall(CallInst *CI);

  void createConstraintsForFunction(Function *F);
  void createConstraintsForInst(Instruction *I); 
  void createConstraintsForPhi(PHINode *Phi); 
  void createConstraintsForCall(CallInst *CI); 
  void createConstraintsForReturn(ReturnInst *RI); 

  void createPointersForFunctionArgs(Function *F); 
  void createPointersForFunction(Function *F); 
  void createPointerForInst(Instruction *I); 

  void createPointerForAlloca(AllocaInst *AI);
  void createPointerForGetElementPtr(GetElementPtrInst *GEP);
  void createPointerForCall(CallInst *CI);
  void createPointerForBitCast(BitCastInst *BCI);
  void createPointerForPhiNode(PHINode *Phi);

  Pointer      *createPointerForGlobal(GlobalValue *GV); 
  BasePointer  *createBasePointer(Value *V);
  BasePointer  *createBasePointer(Value *V, Range R);
  Pointer      *createCallPointer(CallInst *CI, Function *F);
  NoopPointer  *createNoopPointer(Value *V);
  NoopPointer  *createNoopPointer(Value *V, Value *Incoming);
  IndexPointer *createIndexPointer(GetElementPtrInst *GEP);
  PhiPointer   *createPhiPointer(Value *V); 
  BasePointer  *createPointerForType(Value *V, Type *T); 

  Function   *F_;
  BasicBlock *BB_;
  Value      *I_;

  DominatorTree *DT_;
  DominanceFrontier *DF_;
  DataLayout *DL_;
  SymbolicRangeAnalysis *RG_;
  TargetLibraryInfo *TLI_;

  LLVMContext *Context;

  PointersMapTy PointersMap_;
  DenseMap<Value*, Range>    ValueSizes_;
  DenseMap<Function*, Pointer*> ReturnVal_;
};

} // end namespace llvm

#endif

