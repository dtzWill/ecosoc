#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "IneqGraph.h"
#include "PointerSSI.h"
#include "AliasSets.h"

#include <vector>

using namespace llvm;

class ASI : public ModulePass {
  public:
    static char ID;
    ASI() : ModulePass(ID) { }

    Value *getByteSize(Value *V);
    Value *getIndexSize(Value *V);

    bool isSafe(Value *Ptr);
    bool isValidSize(Value *Size);

    virtual bool runOnModule(Module &M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual void print(raw_ostream& OS, const Module *) const;

  private:
    typedef SetVector<Value*> PointersTy;
    typedef DenseMap<Function*, PointersTy*> FnPointersTy;
    typedef DenseMap<Value*, Value*> SizesTy;
    typedef SetVector<BasicBlock*> BlocksTy;
    typedef std::vector<Value*> ArgsValueTy;
    typedef DenseMap<Argument*, ArgsValueTy*> ArgsMapTy;
    typedef SetVector<Value*> ArgsMapHandledCallsTy;

    Value *getSize(SizesTy &Sizes, Value *V);

    void insertSize(SizesTy &Sizes, Value *V, Value *Size);
    void insertByteSize(Value *V, Value *Size);
    void insertIndexSize(Value *V, Value *Size);

    void insertSize(SizesTy &Sizes, Value *V, unsigned Size);
    void insertByteSize(Value *V, unsigned Size);
    void insertIndexSize(Value *V, unsigned Size);

    void insertArgsValue(CallInst *CI, Function *F);

    Value *meetSizes(Value *A, Value *B);

    void inferMallocLikeFn(Function *F);
    void inferAllocationSize(Instruction *I);
    void inferParametersAllocationSize(SizesTy &Sizes, Function *F);

    void visitGlobal(GlobalVariable *GV);
    void visitAlloca(AllocaInst *AI);
    void visitBitCast(BitCastInst *BCI);
    void visitCall(CallInst *CI);
    void visitGetElementPtr(GetElementPtrInst *GEP);
    void visitPhiNode(PHINode *Phi);

    DominatorTree *DT;
    TargetLibraryInfo *TLI;
    IneqGraph *IG;
    DataLayout *DL;
    CallGraph *CG;
    PointerSSI *PSSI;

    LLVMContext *Context;

    FnPointersTy MallocLikeFnPointers;
    FnPointersTy FreeLikeFnPointers;
    SizesTy ByteSizes;
    SizesTy IndexSizes;
    SetVector<BasicBlock*> Worklist;
    SetVector<Function*> FnWorklist;
    ArgsMapTy ArgsMap;
    ArgsMapHandledCallsTy ArgsMapHandledCalls;

    Value *Top;
    Value *Bottom;
};

// For debugging purposes.
class ASIMetadata : public ModulePass {
  public:
    static char ID;
    ASIMetadata() : ModulePass(ID) { } 

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnModule(Module &M);

  private:
    ASI *ASIR;
};

