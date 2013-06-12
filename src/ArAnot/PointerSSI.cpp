#define DEBUG_TYPE "pssi"

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "PointerSSI.h"

using namespace llvm;

using std::map;
using std::set;

StringRef OmegaPrefix = "omega_node";
StringRef PhiPrefix = "phi_node";

void PointerSSI::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AliasSets>();
  AU.addRequired<DominatorTree>();
  AU.addRequired<DominanceFrontier>();
}

bool PointerSSI::runOnModule(Module &M) {
  AS = &getAnalysis<AliasSets>();
  for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F) {
    if (!F->isDeclaration() && !F->isIntrinsic()) {
      // Modifies the CFG so that every function call with at least one pointer
      // argument is at the beginning of a basic block.
      splitCallsInFunction(&(*F));

      // These have to be after the block-splitting part.
      DT = &getAnalysis<DominatorTree>(*F);
      DF = &getAnalysis<DominanceFrontier>(*F);

      // Create sigmas nodes for pointer function arguments and its aliases.
      createSigmasInFunction(&(*F));
    }
  }
  return true;
}

// splitCallsInFunction
// Modifies the CFG so that every function call with at least one pointer
// argument is at the beginning of a basic block.
void PointerSSI::splitCallsInFunction(Function *F) {
  for (Function::iterator BB = F->begin(); BB != F->end(); ++BB)
    for (BasicBlock::iterator I = BB->begin(); I != BB->end(); ++I)
      if (CallInst *CI = dyn_cast<CallInst>(&(*I)))
        if (isPointerFnCall(CI)) {
          // Split the block at the CallInst.
          SplitBlock(BB, CI, this);
          // Resume iterating from the created block.
          ++BB;
        }
}

// createSigmasInFunction
// Create omega nodes at call sites in the given function.
void PointerSSI::createSigmasInFunction(Function *F) {
  for (Function::iterator BB = F->begin(), BE = F->end(); BB != BE; ++BB)
    if (CallInst *CI = dyn_cast<CallInst>(BB->getFirstNonPHI()))
      if (isPointerFnCall(CI))
        for (unsigned Idx = 0; Idx < CI->getNumArgOperands(); ++Idx) {
          Value *Operand = CI->getArgOperand(Idx);
          Type *OperandType = Operand->getType();
          if (OperandType->isPointerTy() && !isa<Constant>(Operand))
            createOmegaNodeAt(Operand, CI->getParent(), CI);
        }
}

// isPointerFnCall
// Returns true if the function call contains at least one pointer argument.
bool PointerSSI::isPointerFnCall(CallInst *CI) {
  for (unsigned Idx = 0; Idx < CI->getNumArgOperands(); ++Idx)
    if (CI->getArgOperand(Idx)->getType()->isPointerTy())
      return true;
  return false;
}

// createOmegaNodeAt
// Creates an omega node for the given Value* at the given BasicBlock*.
void PointerSSI::createOmegaNodeAt(Value *V, BasicBlock *BB, CallInst *CI,
                                   bool InAliasSet) {
  if (!OmegaMap.count(V))
    OmegaMap[V] = new BlockPhiMapTy();

  // Return if the given Value* already has an omega definition at the given
  // BasicBlock*.
  if (OmegaMap[V]->count(BB))
    return;

  // Return if the given Value* is an omega node and the incoming value
  // already has an omega definition at the given BasicBlock*.
  if (PHINode *Omega = dyn_cast<PHINode>(V))
    if (Omega->getNumIncomingValues() == 1 &&
        OmegaMap.count(Omega->getIncomingValue(0)) &&
        OmegaMap[Omega->getIncomingValue(0)]->count(BB))
      return;

  DEBUG(dbgs() << "PointerSSI: createOmegaNodeAt: " << *V << " ==> "
               << BB->getName() << "\n");
  PHINode *Omega = PHINode::Create(V->getType(), 1, OmegaPrefix, BB->begin());
  Omega->addIncoming(V, BB->getSinglePredecessor());
  (*OmegaMap[V])[BB] = Omega;
  DEBUG(dbgs() << "PointerSSI: createOmegaNodeAt: Created: " << *Omega << "\n");

  // Replace the argument V of the call with Omega, to avoid it being replaces
  // by the phi node.
  if (CI)
    CI->replaceUsesOfWith(V, Omega);

  // Insert the omega node into the value map.
  if (ValueMap.count(V))
    ValueMap[Omega] = ValueMap[V];
  else
    ValueMap[Omega] = V;

  // Phi nodes should be created on all blocks in the dominance frontier of BB
  // where V is defined.
  DominanceFrontier::iterator DI = DF->find(BB);
  if (DI != DF->end())
    for (set<BasicBlock*>::iterator BI = DI->second.begin(),
         BE = DI->second.end(); BI != BE; ++BI)
      // If the block in the frontier dominates a use of V, then a phi node
      // should be created at said block.
      if (dominatesUse(V, *BI))
         if (PHINode *Phi = createPhiNodeAt(V, *BI))
           // Replace all incoming definitions with the omega node for every
           // predecessor where the omega node is defined.
           for (pred_iterator PI = pred_begin(*BI), PE = pred_end(*BI);
                PI != PE; ++PI)
             if (DT->dominates(BB, *PI)) {
               Phi->removeIncomingValue(*PI);
               Phi->addIncoming(Omega, *PI);
             }

  // Replace all users of the V with the omega node, starting at BB.
  replaceUsesOfWithAfter(V, Omega, BB);

  // FIXME: Create omegas for Argument* values.
  // Also create omega nodes for all of V's aliases.
  if (!InAliasSet)
    return;

  // Omega nodes aren't in the alias sets, so we have to, instead, use the
  // associated non-omega definition.
  if (ValueMap.count(V))
      V = ValueMap[V];

  if (AS->getValueSetKey(V) == 0)
    return;

  DenseMap<int, set<Value*> > Values = AS->getValueSets();
  set<Value*> Aliases = Values[AS->getValueSetKey(V)];

  for (set<Value*>::iterator VI = Aliases.begin(), VE = Aliases.end();
       VI != VE; ++VI)
    if (*VI != V)
      // If the alias is an instruction and it's defined at the call site,
      // create and omega node for it.
      if (Instruction *I = dyn_cast<Instruction>(*VI))
        if (DT->properlyDominates(I->getParent(), BB))
          createOmegaNodeAt(*VI, BB, NULL, true);
}

// createPhiNodeAT
// Creates a phi node for the given Value* at the given BasicBlock*.
PHINode *PointerSSI::createPhiNodeAt(Value *V, BasicBlock *BB) {
  if (!BlockMap.count(BB))
    BlockMap[BB] = new PhiMapTy();

  // If a phi node already exists for V at BB, return the definition.
  if (BlockMap[BB]->count(V))
    return (*BlockMap[BB])[V];

  if (ValueMap.count(V) && BlockMap[BB]->count(ValueMap[V]))
    return (*BlockMap[BB])[V];

  // Return NULL if V isn't defined on all predecessors of BB.
  if (Instruction *I = dyn_cast<Instruction>(V))
    for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
      if (!DT->dominates(I->getParent(), *PI))
        return NULL;

  DEBUG(dbgs() << "PointerSSI: createPhiNodeAt: " << *V << " at "
               << BB->getName() << "\n");
  PHINode *Phi = PHINode::Create(V->getType(), 1, PhiPrefix, BB->begin());
  (*BlockMap[BB])[V] = Phi;
  DEBUG(dbgs() << "PointerSSI: createPhiNodeAt: Created " << *Phi << "\n");

  // Insert the phi node into the value map.
  if (ValueMap.count(V))
    ValueMap[Phi] = ValueMap[V];
  else
    ValueMap[Phi] = V;

  // Add the default incoming values.
  for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
    Phi->addIncoming(V, *PI);

  // Replace all users of V with the phi node, starting at BB.
  replaceUsesOfWithAfter(V, Phi, BB);

  return Phi;
}

// dominatesUse
// Returns true if BB dominates a use of V.
bool PointerSSI::dominatesUse(Value *V, BasicBlock *BB) {
  for (Value::use_iterator UI = V->use_begin(), UE = V->use_end();
       UI != UE; ++UI)
    // A PHINode* can dominate it's operands, so they have to be disregarded.
    if (isa<PHINode>(*UI) || V == *UI) {
      continue;
    } else if (Instruction *I = dyn_cast<Instruction>(*UI)) {
      // Return true if BB dominates the use.
      if (DT->dominates(BB, I->getParent()))
        return true;
    }
  return false;
}

// replaceUsesOfWithAfter
void PointerSSI::replaceUsesOfWithAfter(Value *V, Value *R, BasicBlock *BB) {
  DEBUG(dbgs() << "PointerSSI: replaceUsesOfWithAfter: " << *V << " ==> "
               << *R << " after " << BB->getName() << "\n");
  SetVector<Instruction*> Replace;
  for (Value::use_iterator UI = V->use_begin(), UE = V->use_end();
       UI != UE; ++UI)
    if (Instruction *I = dyn_cast<Instruction>(*UI)) {
      // If the instruction's parent dominates BB, mark the instruction to
      // be replaced.
      if (I != R && DT->dominates(BB, I->getParent()))
        Replace.insert(I);
      // If parent does not dominate BB, check if the use is a phi and replace
      // the incoming value.
      else if (PHINode *Phi = dyn_cast<PHINode>(*UI))
        for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx)
          if (Phi->getIncomingValue(Idx) == V &&
              DT->dominates(BB, Phi->getIncomingBlock(Idx)))
            Phi->setIncomingValue(Idx, R);
    }

  // Replace V with R on all marked instructions.
  for (SetVector<Instruction*>::iterator I = Replace.begin(),
       IE = Replace.end(); I != IE; ++I)
    (*I)->replaceUsesOfWith(V, R);
}

char PointerSSI::ID = 0;
static RegisterPass<PointerSSI> X("pssi", "SSI for pointers");

