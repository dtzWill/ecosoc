#ifndef LLVM_TRANSFORMS_POINTERSSI_POINTERSSI_H_
#define LLVM_TRANSFORMS_POINTERSSI_POINTERSSI_H_

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "AliasSets.h"

namespace llvm {

class PointerSSI : public ModulePass {
  private:
    typedef DenseMap<Value*, Value*> ValueMapTy;
    typedef DenseMap<Value*, PHINode*> PhiMapTy;
    typedef DenseMap<BasicBlock*, PhiMapTy*> BlockMapTy;
    typedef DenseMap<BasicBlock*, PHINode*> BlockPhiMapTy;
    typedef DenseMap<Value*, BlockPhiMapTy*> OmegaMapTy;

    DominatorTree *DT;
    DominanceFrontier *DF;
    AliasSets *AS;

    // Maps a (BasicBlock*, Value*) to a PHINode*.
    BlockMapTy BlockMap;
    // Maps a Value* to a Value*.
    // Omegas are recursively mapped to their original definition.
    // For example:
    //   %omega_node1 = phi i8* [ %call0, pred ]
    //   %omega_node2 = phi i8* [ omega_node1, other_pred ]
    // Generates the mappings:
    //   %call0       ==> %call0
    //   %omega_node1 ==> %call0
    //   %omega_node2 ==> %call0
    ValueMapTy ValueMap;
    // Maps a (Value*, BasicBlock*) to a PHINode*.
    // This is used to store a value's omega definitions in a given basic block.
    OmegaMapTy OmegaMap;

  public:
    static char ID;
    PointerSSI() : ModulePass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnModule(Module &M);

  private:
    bool isPointerFnCall(CallInst *CI);
    bool dominatesUse(Value *V, BasicBlock *BB);
    void replaceUsesOfWithAfter(Value *V, Value *R, BasicBlock *BB);

    void splitCallsInFunction(Function *F);
    void splitPointerFnCalls(BasicBlock *BB);

    void createSigmasInFunction(Function *F);

    void createOmegaNodeAt(Value *V, BasicBlock *BB, CallInst *CI,
                           bool InAliasSet = false);
    PHINode *createPhiNodeAt(Value *V, BasicBlock *BB);
};

}

#endif

