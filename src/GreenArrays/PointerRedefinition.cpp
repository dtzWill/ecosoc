/*
 * TODO: This is okay for benchmarks such as the ones in SPEC - with no external
 * dependencies other than libc.
 * TODO: Handle globals.
 */

#include "PointerRedefinition.h"
#include "AliasSets.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <queue>
#include <set>

using namespace llvm;

static cl::opt<bool> PtrRedefDebug("ptr-redef-debug",
                                   cl::desc("Enable debugging"),
                                   cl::Hidden, cl::init(false));

static const LibFunc::Func LibFreeLikeFns[] = {
  LibFunc::free // free
};

#define PRDEF_DEBUG(X) { if (PtrRedefDebug) { X; } }

// CreateNamedPhi
static PHINode *CreateNamedPhi(Value *V, Twine Prefix,
                               BasicBlock::iterator Where) {
  Twine Name = Prefix;
  if (V->hasName())
    Name = Prefix + "." + V->getName();
  return PHINode::Create(V->getType(), 1, Name, Where);
}


// CreateNamedBitCast
static BitCastInst *CreateNamedBitCast(Value *V, Twine Prefix,
                                       Instruction *Before) {
  Twine Name = Prefix;
  if (V->hasName())
    Name = Prefix + "." + V->getName();
  return new BitCastInst(V, V->getType(), Name, Before);
}

// getAnalysisUsage
void PointerRedefinition::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTree>();
  AU.addRequired<DominanceFrontier>();
  AU.addRequired<AliasSets>();
  AU.addRequired<TargetLibraryInfo>();
  AU.setPreservesAll();
  //AU.addPreserved<DominatorTree>();
  //AU.addPreserved<DominanceFrontier>();
}

// runOnModule
bool PointerRedefinition::runOnModule(Module &M) {
  TLI_ = &getAnalysis<TargetLibraryInfo>();

  // Insert all the state changers' arguments into the map.
  LLVMContext *Context = &M.getContext();
  for (unsigned Idx = 0; Idx < array_lengthof(LibFreeLikeFns); ++Idx) {
    StringRef Name = TLI_->getName(LibFreeLikeFns[Idx]);
    if (Function *F = M.getFunction(Name)) {
      // Check the function type.
      FunctionType *FTy = F->getFunctionType();
      if (FTy->getReturnType()->isVoidTy() &&
          FTy->getNumParams() == 1 &&
          FTy->getParamType(0) == Type::getInt8PtrTy(*Context)) {
        Value *Arg = &(*F->arg_begin());
        StateChangers_[F].insert(Arg);
      }
    }
  }

  std::queue<Function*> Worklist;
  for (auto& P : StateChangers_)
    Worklist.push(P.first);

  while (!Worklist.empty()) {
    Function *F = Worklist.back(); Worklist.pop();
    for (Value::use_iterator UI = F->use_begin(), UE = F->use_end();
         UI != UE; ++UI) {
      if (Instruction *I = dyn_cast<Instruction>(*UI)) {
        Function *Caller = I->getParent()->getParent();
        if (StateChangers_.count(Caller))
          continue;
        // We assume the changes are not self-contained on all pointer
        // arguments.
        for (Function::arg_iterator AI = Caller->arg_begin(),
             AE = Caller->arg_end(); AI != AE; ++AI)
          if (AI->getType()->isPointerTy())
            StateChangers_[Caller].insert(&(*AI));
        Worklist.push(Caller);
      }
    }
  }

  for (auto& F : M) {
    if (F.isDeclaration() || F.isIntrinsic())
      continue;
    //if (F.getName() != "imp_gauge_action")
    //  continue;
    //errs() << "(PointerRedefinition) Function: " << F.getName() << "\n";
    //errs() << "GETTING ALIAS SET\n";
    AS_ = &getAnalysis<AliasSets>(F);
    //errs() << "GOT\n";
    DT_ = &getAnalysis<DominatorTree>(F);
    //errs() << "DT\n";
    DF_ = &getAnalysis<DominanceFrontier>(F);
    //errs() << "DF\n";
    createRedefsInFunction(&F);
  }

  return true;
}

// doFinalization
bool PointerRedefinition::doFinalization(Module& M) {
  dbgs() << "=================== STATS ===================\n"; 
  dbgs() << "==== Number of create sigmas:   "
         << StatNumCreatedSigmas_       << "\t ====\n";
  dbgs() << "==== Number of create phis:     "
         << StatNumCreatedPhis_ << "\t ====\n";
  return true;
}

// createRedefsInFunction
// Create sigma nodes for all branches in the function.
void PointerRedefinition::createRedefsInFunction(Function *F) {
  DomTreeNode *Node = DT_->getNode(&F->getEntryBlock());
  F_ = F;
  for (po_iterator<DomTreeNode*> BI = po_begin(Node), BE = po_end(Node);
       BI != BE; ++BI) {
    BasicBlock *BB = (*BI)->getBlock();
    for (BasicBlock::reverse_iterator I = BB->rbegin(), IE = BB->rend();
         I != IE; ++I)
      createRedefsForInst(&(*I));
  }
}

// createRedefsForInst
void PointerRedefinition::createRedefsForInst(Instruction *I) {
  unsigned Opcode = I->getOpcode();
  switch (Opcode) {
    //case Instruction::GetElementPtr:
    //  createRedefsForGEP(cast<GetElementPtrInst>(I));
    //  break;
    case Instruction::Call:
      createRedefsForCallInst(cast<CallInst>(I));
      break;
    default:
      break;
  }
}

// createRedefsForCallInst
void PointerRedefinition::createRedefsForCallInst(CallInst *CI) {
  if (CI->isInlineAsm())
    return;

  Function *F = CI->getCalledFunction();
  if (!F) {
    Value *V = CI->getCalledValue();
    if (isa<Constant>(V))
      return;
    //errs() << "CALL ALIASES: " << *V << "\n";
    auto S = AS_->getAliasSet(V);
    for (auto F : *S) {
      //errs() << F->getName() << "\n";
    }
  }

  if (!F || !StateChangers_.count(F))
    return;

  PRDEF_DEBUG(dbgs() << "createRedefsForCallInst: " << *CI << "\n");
  std::set<Value*> Set = StateChangers_[F];
  std::set<Value*> AllAliases;

  for (auto& Arg : Set) {
    unsigned ArgNo = cast<Argument>(Arg)->getArgNo();
    Value *Ptr = CI->getArgOperand(ArgNo);
    if (isa<GlobalValue>(Ptr) || isa<Constant>(Ptr))
      continue;

    StatNumCreatedSigmas_++;
    BitCastInst *Redef = CreateNamedBitCast(Ptr, GetRedefPrefix(), CI);
    PRDEF_DEBUG(dbgs() << "## (Alias) BitCastInst *Redef = " << *Redef << "\n");

    if (std::set<Value*> *Aliases = AS_->getAliasSet(Ptr))
      AllAliases.insert(Aliases->begin(), Aliases->end());
    AS_->bind(Redef, Ptr);

    createPhiNodeForRedef(Redef);

    // Replace all users of the V with the omega node, starting at BB.
    replaceUsesOfWithAfter(Ptr, Redef, CI);

  }

  for (auto& Ptr : AllAliases) {
    if (!dominates(Ptr, CI) || !dominatesUse(CI, Ptr) || CI == Ptr)
      continue;
    StatNumCreatedSigmas_++;
    BitCastInst *Redef = CreateNamedBitCast(Ptr, GetRedefPrefix(), CI);
    PRDEF_DEBUG(dbgs() << "## (Alias) BitCastInst *Redef = " << *Redef << "\n");

    createPhiNodeForRedef(Redef);

    // Replace all users of the V with the omega node, starting at BB.
    replaceUsesOfWithAfter(Ptr, Redef, CI);
  }
}

// createPhiNodeForRedef
void PointerRedefinition::createPhiNodeForRedef(Instruction *I) {
  Value *Ptr = I->getOperand(0);
  BasicBlock *BB = I->getParent();

  // Phi nodes should be created on all blocks in the dominance frontier of BB
  // where V is defined.
  DominanceFrontier::iterator DI = DF_->find(BB);
  if (DI != DF_->end())
    for (std::set<BasicBlock*>::iterator BI = DI->second.begin(),
         BE = DI->second.end(); BI != BE; ++BI) {
      // If the block in the frontier dominates a use of V, then a phi node
      // should be created at said block.
      if (dominatesUse(Ptr, *BI))
         if (PHINode *Phi = createPhiNodeAt(Ptr, *BI)) {
           AS_->bind(Phi, Ptr);
           // Replace all incoming definitions with the omega node for every
           // predecessor where the omega node is defined.
           for (pred_iterator PI = pred_begin(*BI), PE = pred_end(*BI);
                PI != PE; ++PI)
             if (DT_->dominates(BB, *PI)) {
               Phi->removeIncomingValue(*PI);
               Phi->addIncoming(I, *PI);
             }
         }
    }
}

// createPhiNodeAt
// Creates a phi node for the given Value* at the given BasicBlock*.
PHINode *PointerRedefinition::createPhiNodeAt(Value *V, BasicBlock *BB) {
  assert(V->getType()->isPointerTy() && "Value must be a pointer");

  // Return NULL if V isn't defined on all predecessors of BB.
  if (Instruction *I = dyn_cast<Instruction>(V))
    for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
      if (!DT_->dominates(I->getParent(), *PI))
        return NULL;

  PRDEF_DEBUG(dbgs() << "Redefinition: createPhiNodeAt: " << *V << " at "
                    << BB->getName() << "\n");
  StatNumCreatedPhis_;
  PHINode *Phi = CreateNamedPhi(V, GetPhiPrefix(), BB->begin());
  PRDEF_DEBUG(dbgs() << "Redefinition: createPhiNodeAt: Created " << *Phi << "\n");

  // Add the default incoming values.
  for (pred_iterator PI = pred_begin(BB), PE = pred_end(BB); PI != PE; ++PI)
    Phi->addIncoming(V, *PI);

  // Replace all users of V with the phi node, starting at BB.
  replaceUsesOfWithAfter(V, Phi, BB);

  return Phi;
}

// replaceUsesOfWithAfter
void PointerRedefinition::replaceUsesOfWithAfter(Value *V, Value *R, BasicBlock *BB) {
  PRDEF_DEBUG(dbgs() << "Redefinition: replaceUsesOfWithAfter: " << *V << " ==> "
               << *R << " after " << BB->getName() << "\n");
  SetVector<Instruction*> Replace;
  for (Value::use_iterator UI = V->use_begin(), UE = V->use_end();
       UI != UE; ++UI)
    if (Instruction *I = dyn_cast<Instruction>(*UI)) {
      // Don't replace incoming operands at the redefinition block.
      if (isa<PHINode>(I) && I->getParent() == BB)
        continue;
      // If the instruction's parent dominates BB, mark the instruction to
      // be replaced.
      if (I != R && DT_->dominates(BB, I->getParent()))
        Replace.insert(I);
      // If parent does not dominate BB, check if the use is a phi and replace
      // the incoming value.
      else if (PHINode *Phi = dyn_cast<PHINode>(*UI))
        for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx)
          if (Phi->getIncomingValue(Idx) == V &&
              DT_->dominates(BB, Phi->getIncomingBlock(Idx)))
            Phi->setIncomingValue(Idx, R);
    }

  // Replace V with R on all marked instructions.
  for (SetVector<Instruction*>::iterator I = Replace.begin(),
       IE = Replace.end(); I != IE; ++I)
    (*I)->replaceUsesOfWith(V, R);
}

// replaceUsesOfWithAfter
void PointerRedefinition::replaceUsesOfWithAfter(Value *V, Value *R,
                                                 Instruction *I) {
  PRDEF_DEBUG(dbgs() << "replaceUsesOfWithAfter: " << *V
                     << " ==> " << *R << " after " << *I << "\n");

  BasicBlock *BB = I->getParent();
  std::set<Value*> After;
  bool ReachedInst = false;
  for (auto& II : *BB) {
    if (!ReachedInst) {
      if (&II == I)
        ReachedInst = true;
      continue;
    }
    After.insert(&II);
  }

  I->replaceUsesOfWith(V, R);

  SetVector<Instruction*> Replace;
  for (Value::use_iterator UI = V->use_begin(), UE = V->use_end();
       UI != UE; ++UI)
    if (Instruction *I = dyn_cast<Instruction>(*UI)) {
      // Don't replace incoming operands at the redefinition block.
      if (I->getParent() == BB) {
        if (After.count(I))
          Replace.insert(I);
        else
          continue;
      }
      // If the instruction's parent dominates BB, mark the instruction to
      // be replaced.
      else if (I != R && DT_->dominates(BB, I->getParent()))
        Replace.insert(I);
      // If parent does not dominate BB, check if the use is a phi and replace
      // the incoming value.
      else if (PHINode *Phi = dyn_cast<PHINode>(*UI))
        for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx)
          if (Phi->getIncomingValue(Idx) == V &&
              DT_->dominates(BB, Phi->getIncomingBlock(Idx)))
            Phi->setIncomingValue(Idx, R);
    }

  // Replace V with R on all marked instructions.
  for (SetVector<Instruction*>::iterator I = Replace.begin(),
       IE = Replace.end(); I != IE; ++I)
    (*I)->replaceUsesOfWith(V, R);
}

// dominatesUse
// Returns true if BB dominates a use of V.
bool PointerRedefinition::dominatesUse(Value *V, BasicBlock *BB) {
  for (Value::use_iterator UI = V->use_begin(), UE = V->use_end();
       UI != UE; ++UI)
    // A PHINode* can dominate it's operands, so they have to be disregarded.
    if (isa<PHINode>(*UI) || V == *UI) {
      continue;
    } else if (Instruction *I = dyn_cast<Instruction>(*UI)) {
      // Return true if BB dominates the use.
      if (DT_->dominates(BB, I->getParent()))
        return true;
    }
  return false;
}

// dominatesUse
// Returns true if BB dominates a use of V.
bool PointerRedefinition::dominatesUse(Value *V, Value *U) {
  std::set<Instruction*> BlockUses;
  if (Instruction *I = dyn_cast<Instruction>(V)) {
    bool DoInsert = false;
    for (auto& II : *I->getParent())
      if (&II == I) 
        DoInsert = true;
      else if (DoInsert)
        BlockUses.insert(&II);
  }

  BasicBlock *BB;
  if (Argument *A = dyn_cast<Argument>(V))
    BB = &A->getParent()->getEntryBlock();
  else if (Instruction *I = dyn_cast<Instruction>(V))
    BB = I->getParent();
  else
    assert(false);

  for (Value::use_iterator UI = U->use_begin(), UE = U->use_end();
       UI != UE; ++UI)
    // A PHINode* can dominate it's operands, so they have to be disregarded.
    if (isa<PHINode>(*UI) || V == *UI) {
      continue;
    } else if (Instruction *I = dyn_cast<Instruction>(*UI)) {
      // Return true if BB dominates the use.
      if (DT_->dominates(BB, I->getParent()) && BB != I->getParent()) {
        // errs() << BB->getName() << " dominates " << I->getParent()->getName() << "\n";
        return true;
      }
      else if (BlockUses.count(I)) {
        // errs() << *I << " is in the BlockUses of " << *V << "\n";
        return true;
      }
    }
  return false;
}

// dominatesUse
//
// Returns true if BB dominates a use of V.
bool PointerRedefinition::dominates(Value *Def, Value *V) {
  if (isa<Argument>(Def) || isa<Argument>(V))
    return true;
  Instruction *IDef = cast<Instruction>(Def);
  Instruction *I = cast<Instruction>(V);
  if (IDef->getParent() != I->getParent())
    return DT_->dominates(IDef->getParent(), I->getParent());
  for (auto& II : *I->getParent())
    if (&II == IDef)
      return true;
    else if (&II == I)
      return false;
  assert(false);
}

char PointerRedefinition::ID = 0;
static RegisterPass<PointerRedefinition>
  X("ptr-redef", "Pointer redefinition for sparse abstract interperatation");

