#define DEBUG_TYPE "asi"

#include "llvm/DebugInfo.h"
#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLibraryInfo.h"

#include "ArraySizeInference.h"

using namespace llvm;

STATISTIC(TotalMemIns, "Total memory instructions");
STATISTIC(SafeMemIns,  "Safe memory instructions");

static bool IsSLT(APInt L, APInt R) {
  unsigned LBitWidth = L.getBitWidth();
  unsigned RBitWidth = R.getBitWidth();
  unsigned MaxBitWidth = LBitWidth > RBitWidth ? LBitWidth : RBitWidth;
  return L.sextOrSelf(MaxBitWidth).slt(R.sextOrSelf(MaxBitWidth));
}

// Memory allocation functions for C and C++.
static const LibFunc::Func LibMallocLikeFns[] = {
  LibFunc::malloc,
  LibFunc::valloc,
  LibFunc::Znwj,               // new(unsigned int)
  LibFunc::ZnwjRKSt9nothrow_t, // new(unsigned int, nothrow)
  LibFunc::Znwm,               // new(unsigned long)
  LibFunc::ZnwmRKSt9nothrow_t, // new(unsigned long, nothrow)
  LibFunc::Znaj,               // new[](unsigned int)
  LibFunc::ZnajRKSt9nothrow_t, // new[](unsigned int, nothrow)
  LibFunc::Znam,               // new[](unsigned long)
  LibFunc::ZnamRKSt9nothrow_t, // new[](unsigned long, nothrow)
};

// Deallocation functions for C and C++.
// TODO:
// Add C++'s del[].
static const LibFunc::Func LibFreeLikeFns[] = {
  LibFunc::free                // free
};

void ASI::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<PointerSSI>();
  AU.addRequired<DataLayout>();
  AU.addRequired<DominatorTree>();
  AU.addRequired<TargetLibraryInfo>();
  AU.addRequired<IneqGraph>();
  AU.addRequired<CallGraph>();
  AU.setPreservesAll();
}

bool ASI::runOnModule(Module &M) {
  PSSI = &getAnalysis<PointerSSI>();
  DL = &getAnalysis<DataLayout>();
  TLI = &getAnalysis<TargetLibraryInfo>();
  IG = &getAnalysis<IneqGraph>();
  CG = &getAnalysis<CallGraph>();
  
  Context = &M.getContext();

  // Symbolic top and bottom for the array-size lattice.
  Top = ConstantDataArray::getString(*Context, "** Top **");
  Bottom = ConstantDataArray::getString(*Context, "** Bottom **");

  // Insert the functions in LibMallocLikeFns into MallocLikeFnSize, mapping all
  // the function sizes to each of their first parameter.
  for (unsigned Idx = 0; Idx < array_lengthof(LibMallocLikeFns); ++Idx) {
    StringRef FName = TLI->getName(LibMallocLikeFns[Idx]);
    if (Function *F = M.getFunction(FName)) {
      // Check the function type.
      FunctionType *FTy = F->getFunctionType();
      if (FTy->getReturnType() == Type::getInt8PtrTy(*Context) &&
          FTy->getNumParams() == 1 &&
          (FTy->getParamType(0)->isIntegerTy(32) ||
          FTy->getParamType(0)->isIntegerTy(64))) {
        PointersTy *Pointers = new PointersTy();
        Pointers->insert(&(*F->arg_begin()));
        MallocLikeFnPointers[F] = Pointers;
      }
    }
  }

  // Insert the functions in LibFreeLikeFns into FreeLikeFnPtr, mapping all
  // the frees to each of their first parameter.
  for (unsigned Idx = 0; Idx < array_lengthof(LibFreeLikeFns); ++Idx) {
    StringRef FName = TLI->getName(LibFreeLikeFns[Idx]);
    if (Function *F = M.getFunction(FName)) {
      // Check the function type.
      FunctionType *FTy = F->getFunctionType();
      if (FTy->getReturnType()->isVoidTy() &&
          FTy->getNumParams() == 1 &&
          FTy->getParamType(0) == Type::getInt8PtrTy(*Context)) {
        PointersTy *Pointers = new PointersTy();
        Pointers->insert(&(*F->arg_begin()));
        FreeLikeFnPointers[F] = Pointers;
      }
    }
  }

  // Initialize the ArgsMap for every formal.
  for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F)
    if (!F->isDeclaration() && !F->isIntrinsic())
      for (Function::arg_iterator AI = F->arg_begin(), AE = F->arg_end();
           AI != AE; ++AI)
        ArgsMap[&(*AI)] = new ArgsValueTy();

  // Infer allocation sizes for globals.
  for (Module::global_iterator GI = M.global_begin(), GE = M.global_end();
       GI != GE; ++GI)
    visitGlobal(&(*GI));

  // Add functions to the function worklist in post-order according to the
  // call graph.
  CallGraphNode *Entry = CG->getExternalCallingNode();
  for (po_iterator<CallGraphNode*> FI = po_begin(Entry), FE = po_end(Entry);
       FI != FE; ++FI) {
    Function *F = FI->getFunction();
    if (F && !F->isDeclaration() && !F->isIntrinsic())
      FnWorklist.insert(F);
  }

  while (!FnWorklist.empty()) {
    Function *F = FnWorklist.back();
    FnWorklist.pop_back();

    DEBUG(dbgs() << "ASI: Function: " << F->getName() << "\n");
    errs() << "ASI: Function: " << F->getName() << "\n";

    // Infer the allocation size of the formals.
    inferParametersAllocationSize(ByteSizes, F);
    inferParametersAllocationSize(IndexSizes, F);

    DT = &getAnalysis<DominatorTree>(*F);

    // Initialize the worklist with the basic blocks in the function.
    DomTreeNode *Entry = DT->getNode(&(*F->begin()));
    for (po_iterator<DomTreeNode*> BB = po_begin(Entry), BE = po_end(Entry);
         BB != BE; ++BB)
      Worklist.insert(BB->getBlock());

    // Iterator through the worklist to infer instruction array sizes.
    while (!Worklist.empty()) {
      BasicBlock *BB = Worklist.back();
      Worklist.pop_back();
      for (BasicBlock::iterator I = BB->begin(), IE = BB->end(); I != IE; ++I)
        inferAllocationSize(&(*I));
    }

    // Try to infer if the function returns a call to malloc() or another
    // malloc()-like function.
    inferMallocLikeFn(F);

    // TODO:
    // Come back with free()-like functions inference.
    // inferFreeLikeFn(F);
  }

  if (!AreStatisticsEnabled())
    return false;

  // Count the number of safe memory instructions if statistics are enabled.
  // TODO:
  // Correctly handle constant expressions.
  for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F) {
    if (F->isDeclaration() || F->isIntrinsic())
      continue;
    for (Function::iterator BB = F->begin(), BE = F->end(); BB != BE; ++BB)
      for (BasicBlock::iterator I = BB->begin(), IE = BB->end(); I != IE; ++I) {
        Value *Pointer = NULL;
        if (StoreInst *SI = dyn_cast<StoreInst>(&(*I)))
          Pointer = SI->getPointerOperand();
        else if (LoadInst *LI = dyn_cast<LoadInst>(&(*I)))
          Pointer = LI->getPointerOperand();
        else if (isa<ConstantExpr>(&(*I))) {
          SafeMemIns++;
          TotalMemIns++;
        }
        if (Pointer) {
          if (isSafe(Pointer))
            SafeMemIns++;
          TotalMemIns++;
        }
      }
  }

  return false;
}

// isSafe
bool ASI::isSafe(Value *Ptr) {
  if (Ptr->getType()->isPointerTy()) {
    Type *T = Ptr->getType()->getPointerElementType();
    Value *ByteSize = getByteSize(Ptr);
    // We can load or store from a value if it's byte size is greater the size
    // of it's type...
    if (isValidSize(ByteSize) && T->isSized()) {
      unsigned Size = DL->getTypeAllocSize(T);
      if (IG->getLower(ByteSize).sge(Size))
        return true;
    }
    // ... or if it's index size is strictly positive.
    Value *IndexSize = getIndexSize(Ptr);
    if (isValidSize(IndexSize))
      if (IG->getLower(IndexSize).isStrictlyPositive())
        return true;
  }
  return false;
}

// insertSize
void ASI::insertSize(SizesTy &Sizes, Value *V, Value *Size) {
  DEBUG(dbgs() << "ASI: insertSize: " << *V << " ==> " << *Size << "\n");
  // If V ==> Size, return.
  if (Sizes.count(V) && (Sizes[V] == Size || Sizes[V] == Top))
    return;
  if (ConstantInt *CI = dyn_cast<ConstantInt>(Size)) {
    // If the new size is a constant less than the current size, insert top.
    if (CI->getValue().isStrictlyPositive()) {
      if (Value *VS = Sizes[V])
        if (ConstantInt *CIV = dyn_cast<ConstantInt>(VS))
          if (IsSLT(CIV->getValue(), CI->getValue()))
            Size = Top;
    // Insert top instead of negatives integers.
    } // else {
    //  Size = Top;
    //}
  }

  // Insert the successors in the worklist.
  if (Instruction *I = dyn_cast<Instruction>(V))
    if (Sizes[V] != Size)
      for (succ_iterator Succ = succ_begin(I->getParent()),
           E = succ_end(I->getParent()); Succ != E; ++Succ)
        Worklist.insert(*Succ);

  // Set V ==> Size.
  Sizes[V] = Size;
}

// insertSize
void ASI::insertSize(SizesTy &Sizes, Value *V, unsigned Size) {
  IntegerType *Int32Ty = IntegerType::getInt32Ty(V->getContext());
  Value *ConstSize = ConstantInt::getSigned(Int32Ty, Size);
  insertSize(Sizes, V, ConstSize);
}

// insertIndexSize
void ASI::insertByteSize(Value *V, Value *Size) {
  DEBUG(dbgs() << "ASI: insertByteSize: " << *V << " ==> " << *Size << "\n");
  insertSize(ByteSizes, V, Size);
}

// insertIndexSize
void ASI::insertIndexSize(Value *V, Value *Size) {
  DEBUG(dbgs() << "ASI: insertIndexSize: " << *V << " ==> " << *Size << "\n");
  insertSize(IndexSizes, V, Size);
}

// insertByteSize
void ASI::insertByteSize(Value *V, unsigned Size) {
  DEBUG(dbgs() << "ASI: insertByteSize: " << *V << " ==> " << Size << "\n");
  insertSize(ByteSizes, V, Size);
}

// insertIndexSize
void ASI::insertIndexSize(Value *V, unsigned Size) {
  DEBUG(dbgs() << "ASI: insertIndexSize: " << *V << " ==> " << Size << "\n");
  insertSize(IndexSizes, V, Size);
}

// getSize
Value *ASI::getSize(SizesTy &Sizes, Value *V) {
  return Sizes.count(V) ? Sizes[V] : Bottom;
}

// getByteSize
Value *ASI::getByteSize(Value *V) {
  return getSize(ByteSizes, V);
}

// getIndexSize
Value *ASI::getIndexSize(Value *V) {
  return getSize(IndexSizes, V);
}

// isValidSize
bool ASI::isValidSize(Value *Size) {
  return Size && Size != Top && Size != Bottom;
}

// meetSizess
Value *ASI::meetSizes(Value *A, Value *B) {
  if (A == Top || B == Top)
    return Top;
  if (A == Bottom)
    return B;
  if (B == Bottom)
    return A;
  // A <= B
  if (IG->findShortestPath(A, B).sle(0))
    return A;
  // B <= A
  if (IG->findShortestPath(B, A).sle(0))
    return B;
  return Top;
}

void ASI::inferParametersAllocationSize(SizesTy &Sizes, Function *F) {
  DEBUG(dbgs() << "ASI: inferParametersAllocationSize: " << F->getName() << "\n");
  // For every argument...
  for (Function::arg_iterator AI = F->arg_begin(), AE = F->arg_end();
       AI != AE; ++AI) {
    if (!AI->getType()->isPointerTy())
      continue;
    DEBUG(dbgs() << "ASI: Argument: " << *AI << "\n");
    // Get the set of real arguments for the formal.
    ArgsValueTy *ArgsValues = ArgsMap[&(*AI)];
    // Calculate the meet of all sizes...
    Value *SmallestSize = Bottom;
    for (unsigned Idx = 0; Idx < ArgsValues->size(); ++Idx) {
      Value *Pointer = (*ArgsValues)[Idx];
      Value *ActualSize = getSize(Sizes, Pointer);
      DEBUG(dbgs() << "ActualSize: " << *ActualSize << "\n");
      SmallestSize = meetSizes(SmallestSize, ActualSize);
    }
    DEBUG(dbgs() << "ASI: SmallestSize: " << *SmallestSize << "\n");
    // If the meet is a valid size, set it as the array size  and return.
    if (isValidSize(SmallestSize)) {
      insertSize(Sizes, &(*AI), SmallestSize);
      return;
    }
    // For every other non-pointer argument...
    for (Function::arg_iterator SAI = F->arg_begin(); SAI != AE; ++SAI) {
      if (SAI->getType()->isPointerTy())
        continue;
      DEBUG(dbgs() << "ASI: Other argument: " << *SAI << "\n");
      ArgsValueTy *SArgsValues = ArgsMap[&(*SAI)];
      bool AllLessThan = true && (SArgsValues->size() != 0);
      // Check if the other formal's real argument is less than AI's real size.
      for (unsigned Idx = 0; Idx < SArgsValues->size(); ++Idx) {
        Value *Real = (*ArgsValues)[Idx];
        Value *PossibleSize = (*SArgsValues)[Idx];
        Value *ActualSize = getSize(Sizes, Real);
        if (isValidSize(PossibleSize) && isValidSize(ActualSize)) {
          APInt Path = IG->findShortestPath(PossibleSize, ActualSize);
          DEBUG(dbgs() << "ASI: Path: " << Path << "\n");
          if (!Path.isStrictlyPositive())
            continue;
        }
        AllLessThan = false;
        break;
      }
      // Set the other formal as AI's size if so.
      if (AllLessThan) {
        insertSize(Sizes, &(*AI), &(*SAI));
        return;
      }
    }
  }
}

// inferAllocationSize
void ASI::inferAllocationSize(Instruction *I) {
  DEBUG(dbgs() << "ASI: Try Infer: " << *I << "\n");
  if (!isa<CallInst>(I) && !I->getType()->isPointerTy())
    return;

  DEBUG(dbgs() << "ASI: Infer: " << *I << "\n");

  if (AllocaInst *AI = dyn_cast<AllocaInst>(I))
    visitAlloca(AI);
  else if (CallInst *CI = dyn_cast<CallInst>(I))
    visitCall(CI);
  else if (BitCastInst *BCI = dyn_cast<BitCastInst>(I))
    visitBitCast(BCI);
  else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(I))
    visitGetElementPtr(GEP);
  else if (PHINode *Phi = dyn_cast<PHINode>(I))
    visitPhiNode(Phi);
}


void ASI::visitGetElementPtr(GetElementPtrInst *GEP) {
  const unsigned BitWidth = 64;

  Value *Pointer = GEP->getPointerOperand();
  Value *ByteSize = getByteSize(Pointer);
  Value *IndexSize = getIndexSize(Pointer);

  DEBUG(dbgs() << "ASI: ByteSize: " << *ByteSize << "\n");
  DEBUG(dbgs() << "ASI: IndexSize: " << *IndexSize << "\n");

  // Calculate the offset given by all but the last index.
  APInt Offset(BitWidth, 0, true);
  Type *PointerTy = Pointer->getType();
  for (unsigned Idx = 1; Idx < GEP->getNumOperands() - 1; ++Idx) {
    Type *ContainedType = PointerTy;
    if (PointerType *PT = dyn_cast<PointerType>(ContainedType))
      PointerTy = PT->getElementType();
    else if (ArrayType *AT = dyn_cast<ArrayType>(ContainedType))
      PointerTy = AT->getElementType();
    unsigned AllocSize = DL->getTypeAllocSize(ContainedType);
    APInt IndexInt = cast<ConstantInt>(GEP->getOperand(Idx))->getValue();
    APInt AllocSizeInt = APInt(BitWidth, AllocSize, true);
    Offset += IndexInt.sextOrSelf(BitWidth) * AllocSizeInt;
  }

  DEBUG(dbgs() << "ASI: Offset: " << Offset << "\n");

  // The last index.
  Value *Index = GEP->getOperand(GEP->getNumOperands() - 1);
  unsigned AllocSize =
    DL->getTypeAllocSize(GEP->getType()->getPointerElementType());
  APInt AllocSizeInt = APInt(BitWidth, AllocSize, true);

  if (isValidSize(ByteSize)) {
    // Lower bound of the byte size at the GEP's parent.
    APInt Size = IG->getLowerAt(ByteSize, GEP->getParent());
    DEBUG(dbgs() << "ASI: Size: " << Size << "\n");
    if (!Size.isMaxSignedValue()) {
      // Lower and upper bounds fore the last index.
      APInt LowerIndex = IG->getLower(Index);
      APInt UpperIndex = IG->getUpper(Index);
      DEBUG(dbgs() << "ASI: LowerIndex: " << LowerIndex << "\n");
      DEBUG(dbgs() << "ASI: UpperIndex: " << UpperIndex << "\n");

      APInt TotalOffset = Offset;
      // Calculate the offset that the last index gives from the pointer.
      // If the GEP returns a pointer to a structure, the last index specifies
      // a field withing the structure, so we count the size of all the previous
      // fields.
      if (StructType *ST = dyn_cast<StructType>(PointerTy)) {
        uint64_t IndexInt = UpperIndex.getSExtValue();
        for (unsigned Idx = 0; Idx < IndexInt; ++Idx) {
          Type *T = ST->getElementType(Idx);
          TotalOffset += APInt(BitWidth, DL->getTypeAllocSize(T), true);
        }
      // If the GEP isn't indexing a structure, the offset is given by the upper
      // bound of the index times the GEP's pointer element type.
      } else {
        TotalOffset = UpperIndex.sextOrSelf(BitWidth) * AllocSizeInt;
      }
      // If the upper bound is not +inf and and lower bound isn't negative,
      // insert the byte size, given by the pointers sizes minus the total
      // offset of the access.
      if (!UpperIndex.isMaxSignedValue() && LowerIndex.isNonNegative()) {
        APInt InsertSize = Size.sextOrSelf(BitWidth) - TotalOffset;
        insertByteSize(GEP, InsertSize.getSExtValue());
      }
    }
  }

  // Structs are only handled with byte sizes.
  if (isa<StructType>(PointerTy))
    return;

  if (isValidSize(IndexSize)) {
    APInt Size = IG->findShortestPathAt(Index, IndexSize, GEP->getParent());
    DEBUG(dbgs() << "ASI: Path: " << Size << "\n");
    /* DEBUG(dbgs() << "ASI: index_size(" << *GEP << ") = path(" << *Index
                    << ", "<< *IndexSize << ") - " << Offset << " == " << Size
                    << " - " << Offset << " == "
                    << Size.sextOrSelf(64) - Offset << "\n"); */
    APInt LowerIndex = IG->getLower(Index);
    DEBUG(dbgs() << "ASI: LowerIndex: " << LowerIndex << "\n");
    // Adjust the number of available indecies with the offset of the access. 
    APInt Adjustment = Offset.sdiv(AllocSizeInt) +
                       (Offset.srem(AllocSizeInt) != 0 ? 1 : 0);
    DEBUG(dbgs() << "ASI: Adjustment: " << Adjustment << "\n");
    if (!Size.isMaxSignedValue() && LowerIndex.isNonNegative()) {
      APInt InsertSize = -(Size.sextOrSelf(BitWidth) - Adjustment);
      insertIndexSize(GEP, InsertSize.getSExtValue());
    }
  }
}

// visitGlobal
void ASI::visitGlobal(GlobalVariable *GV) {
  Type *T = GV->getType()->getPointerElementType();
  unsigned Size = DL->getTypeAllocSize(T);
  insertByteSize(GV, Size);
  if (StructType *ST = dyn_cast<StructType>(T))
    insertIndexSize(GV, ST->getStructNumElements());
  else if (ArrayType *AT = dyn_cast<ArrayType>(T))
    insertIndexSize(GV, AT->getNumElements());
}

// visitAlloca
void ASI::visitAlloca(AllocaInst *AI) {
  Type *T = AI->getAllocatedType();
  unsigned Size = DL->getTypeAllocSize(T);
  insertByteSize(AI, Size);
  if (StructType *ST = dyn_cast<StructType>(T))
    insertIndexSize(AI, ST->getStructNumElements());
  else if (ArrayType *AT = dyn_cast<ArrayType>(T))
    insertIndexSize(AI, AT->getNumElements());
}

// visitCall
void ASI::visitCall(CallInst *CI) {
  // TODO: Check if the callee must alias a malloc()-like and infer the
  //       allocation size based on the alias.
  // FIXME: Check if may alias a free()-like function or a realloc()-like
  //        and remove or change the allocation size accordingly.
  // FIXME: If an allocated pointer is passed to an unknown function, consider
  //        consider the pointer deallocated.
  Value *V = CI->getCalledValue();
  Function *Callee = CI->getCalledFunction();
  
  // Check if the called value is a bitcast of a function.
  if (ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
    if (CE->getOpcode() == Instruction::BitCast)
      Callee = dyn_cast<Function>(CE->getOperand(0));
  }

  if (!Callee || Callee->isVarArg())
    return;

  // Handle functions that return a call to malloc() or malloc() itself.
  if (CI->getType()->isPointerTy() && MallocLikeFnPointers.count(Callee)) {
    if (PointersTy *Pointers = MallocLikeFnPointers[Callee]) {
      for (PointersTy::iterator PI = Pointers->begin(), PE = Pointers->end(); PI != PE;
           ++PI) {
        // If the allocation size is an formal parameter, get associated real
        // parameter of the function call.
        if (Argument *FormalSizeArg = dyn_cast<Argument>(*PI)) {
          unsigned FormalSizeArgNo = FormalSizeArg->getArgNo();
          Value *RealSizeArg = CI->getArgOperand(FormalSizeArgNo);
          if (!isValidSize(RealSizeArg))
            continue;
          insertByteSize(CI, RealSizeArg);
          unsigned ElementSize = DL->getTypeAllocSize(CI->getType()->getPointerElementType());
          Value *Multiple = NULL;
          if (ComputeMultiple(RealSizeArg, ElementSize, Multiple, true))
            insertIndexSize(CI, Multiple);
        } else if (isa<ConstantInt>(*PI)) {
          insertByteSize(CI, *PI);
          unsigned ElementSize = DL->getTypeAllocSize(CI->getType()->getPointerElementType());
          Value *Multiple = NULL;
          if (ComputeMultiple(*PI, ElementSize, Multiple, true))
            insertIndexSize(CI, Multiple);
        }
      }
    }
  }

  // If the function is defined in this module, save the arguments of the call.
  if (!Callee->isDeclaration() && !Callee->isIntrinsic())
    insertArgsValue(CI, Callee);

}

void ASI::insertArgsValue(CallInst *CI, Function *F) {
  if (ArgsMapHandledCalls.count(CI))
    return;
  ArgsMapHandledCalls.insert(CI);
  DEBUG(dbgs() << "ASI: insertArgsValue: " << *CI << "\n");
  unsigned Idx = 0;
  for (Function::arg_iterator AI = F->arg_begin(), AE = F->arg_end();
       AI != AE; ++AI, ++Idx)
    ArgsMap[&(*AI)]->push_back(CI->getArgOperand(Idx));
  // FIXME: Possible problem with recursive functions.
  // FnWorklist.insert(A->getParent());
}

// visitBitCast
void ASI::visitBitCast(BitCastInst *BCI) {
  if (!BCI->getType()->isPointerTy())
    return;

  Value *Pointer = BCI->getOperand(0);
  Value *Size = getByteSize(Pointer);
  insertByteSize(BCI, Size);

  Type *BCIT = BCI->getType();
  if (isa<PointerType>(BCIT)) {
    if (!isValidSize(Size))
      return;
    unsigned ElementSize = DL->getTypeAllocSize(BCIT->getPointerElementType());
    Value *Multiple = NULL;
    if (ComputeMultiple(Size, ElementSize, Multiple, true))
      insertIndexSize(BCI, Multiple);
  }
}

// visitPhiNode
void ASI::visitPhiNode(PHINode *Phi) {
  Value *ByteMeet = Bottom;
  for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx) {
    Value *ByteSize = getByteSize(Phi->getIncomingValue(Idx));
    ByteMeet = meetSizes(ByteMeet, ByteSize);
  }
  insertByteSize(Phi, ByteMeet);

  Value *IndexMeet = Bottom;
  for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx) {
    Value *IndexSize = getIndexSize(Phi->getIncomingValue(Idx));
    IndexMeet = meetSizes(IndexMeet, IndexSize);
  }
  insertIndexSize(Phi, IndexMeet);
}

void ASI::inferMallocLikeFn(Function *F) {
  DEBUG(dbgs() << "ASI: inferMallocLikeFn: " << F->getName() << "\n");
  Type *ReturnType = F->getReturnType();

  if (!ReturnType || !ReturnType->isPointerTy())
    return;

  Value *MallocByteArg = NULL;
  for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
    ReturnInst *RI = dyn_cast<ReturnInst>(BB->getTerminator());
    if (!RI)
      continue;
    // Make sure all return values are allocations.
    Instruction *ReturnVal = dyn_cast<Instruction>(RI->getReturnValue());
    if (!ReturnVal)
      return;
    // Check that all allocations have the same size.
    if (MallocByteArg && MallocByteArg != getByteSize(ReturnVal))
      return;
    else
      MallocByteArg = getByteSize(ReturnVal);
  }

  if (!MallocByteArg || !isa<ConstantInt>(MallocByteArg))
    return;

  // If the size of all the allocation is given by one of the arguments,
  // insert the function and the argument into the MallocLikeFn map.
  DEBUG(dbgs() << "ASI: Inferred: " << *MallocByteArg << "\n");
  if (!MallocLikeFnPointers.count(F))
    MallocLikeFnPointers[F] = new PointersTy();
  PointersTy *Pointers = MallocLikeFnPointers[F];
  Pointers->insert(MallocByteArg);
  // Insert all callers to the function worklist.
  // FIXME:
  // Possible problem with function pointers.
  // for (Value::use_iterator UI = F->use_begin(), UE = F->use_end();
  //     UI != UE; ++UI)
  //  if (CallInst *CI = dyn_cast<CallInst>(*UI))
  //    FnWorklist.insert(CI->getParent()->getParent());
}

void ASI::print(raw_ostream &OS, const Module *) const {
  ASI &ASIR = *const_cast<ASI *>(this);
  OS << "-------------------------------------------------------------\n";
  OS << "ArraySize:\n";
  OS << "  Byte sizes:\n";
  for (SizesTy::iterator SI = ASIR.ByteSizes.begin(), SE = ASIR.ByteSizes.end();
       SI != SE; ++SI)
    if (ASIR.isValidSize(SI->second) && !isa<GlobalValue>(SI->first))
      OS << "    " << *SI->first << " ==> " << *SI->second << "\n";
  OS << "\n";
  OS << "  Index sizes:\n";
  for (SizesTy::iterator SI = ASIR.IndexSizes.begin(), SE = ASIR.IndexSizes.end();
       SI != SE; ++SI)
    if (ASIR.isValidSize(SI->second) && !isa<GlobalValue>(SI->first))
      OS << "    " << *SI->first << " ==> " << *SI->second << "\n";
  OS << "\n";
}

char ASI::ID = 0;
static RegisterPass<ASI> X("asi", "Array-size inference");
 
void ASIMetadata::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<ASI>();
  AU.setPreservesAll();
}

bool ASIMetadata::runOnModule(Module &M) {
  ASIR = &getAnalysis<ASI>();

  for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F) {
    if (F->isDeclaration() || F->isIntrinsic())
      continue;
    for (Function::iterator BB = F->begin(), BE = F->end(); BB != BE; ++BB)
      for (BasicBlock::iterator I = BB->begin(), IE = BB->end(); I != IE; ++I) {
        if (Value *Size = ASIR->getByteSize(&(*I)))
          if (ASIR->isValidSize(Size))
            I->setMetadata("byte_size", MDNode::get(I->getContext(), ArrayRef<Value *>(Size)));
        if (Value *Size = ASIR->getIndexSize(&(*I)))
          if (ASIR->isValidSize(Size))
            I->setMetadata("index_size", MDNode::get(I->getContext(), ArrayRef<Value *>(Size)));
      }
  }

  return false;
}

char ASIMetadata::ID = 0;
static RegisterPass<ASIMetadata> Y("asi-metadata", "Emits metadata for the array-size inference");

