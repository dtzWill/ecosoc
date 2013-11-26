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

#include <chrono>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

/* ************************************************************************** */
/* ************************************************************************** */

namespace llvm {

raw_ostream& operator<<(raw_ostream& OS, const Pointer& J) {
  J.print(OS);
  return OS;
}

raw_ostream& operator<<(raw_ostream& OS, const RegionAnalysis& R) {
  R.print(OS);
  return OS;
}

} // end namespace llvm

/* ************************************************************************** */
/* ************************************************************************** */

using namespace llvm;
using std::find;
using std::make_pair;
using std::ostringstream;
using std::pair;
using std::queue;
using std::set;
using std::setprecision;
using std::string;
using std::vector;

static cl::opt<bool>
  RaDebug("ra-debug",
          cl::desc("Enable debugging for the region analysis"),
          cl::Hidden, cl::init(false));

static cl::opt<bool>
  RaDebugEval("ra-debug-eval",
               cl::desc("Enable debugging of the eval() function for the "
                        "region analysis"),
               cl::Hidden, cl::init(false));

static cl::opt<string>
  RaDebugFunction("ra-debug-function",
                  cl::desc("Run and debug the region analysis "
                           "with a specific function"),
                  cl::Hidden, cl::init(""));

static cl::opt<bool>
  RaStats("ra-stats",
          cl::desc("Gather stats for the region analysis"),
          cl::Hidden, cl::init(false));

char RegionAnalysis::ID = 0;
static RegisterPass<RegionAnalysis>
  X("region-analysis", "Region analysis considering libc allocation semantics",
    true, true);

#define RG_DEBUG(X)  { if (RaDebug) { X; } }
#define RG_ASSERT(X) { if (!(X)) { printInfo(errs()); assert(X); } }

#define ASSERT_EQ(X, Y, M)   __assert_eq(X, Y, M, __LINE__, __func__, __FILE__)
#define ASSERT_NE(X, Y, M)   __assert_ne(X, Y, M, __LINE__, __func__, __FILE__)
#define ASSERT_TYPE(X, Y, M) __assert_type<Y>(X, #Y, M, __LINE__, __func__, __FILE__)

/* ************************************************************************** */
/* ************************************************************************** */

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

// __assert_eq
template <class T>
static void __assert_eq(T Left, T Right, const char *Msg,
                        int Line, const char *Func, const char *File) {
  if (Left == Right)
    return;
  errs() << "Assertion failed: " << Left << " == " << Right << "\n";
  errs() << "                  " << Msg << "\n";
  assert(false);
}

// __assert_ne
template <class T>
static void __assert_ne(T Left, T Right, const char *Msg,
                        int Line, const char *Func, const char *File) {
  if (Left != Right)
    return;
  errs() << "Assertion failed: " << Left << " != " << Right << "\n";
  errs() << "                  " << Msg << "\n";
  assert(false);
}

// __assert_type
template <typename T>
static void __assert_type(Value *V, const char *TypeRepr, const char *Msg,
                          int Line, const char *Func, const char *File) {
  if (isa<T>(V))
    return;
  errs() << "Assertion failed: isa<" << TypeRepr << ">(" << *V << ")\n";
  errs() << "                  " << Msg << "\n";
  assert(false);
}

// PrintValue
static void PrintValue(raw_ostream& OS, Value *V) {
  if (V->hasName())
    OS << V->getName();
  else
    OS << *V;
}

// GetMeet
Range GetMeet(Pointer::ArgsTy Args) {
  Range R;
  for (auto& P : Args)
    if (P == *Args.begin())
      R = P->getRange();
    else
      R = R.meet(P->getRange());
  return R;
}

// GetEvalMeet
Range GetEvalMeet(Pointer::ArgsTy Args) {
  Range R;
  for (auto& P : Args)
    if (P == *Args.begin())
      R = P->eval();
    else
      R = R.meet(P->eval());
  return R;
}

/* ************************************************************************** */
/* ************************************************************************** */

/***********
 * Pointer *
 ***********/
Pointer::Pointer(Value *V)
  : Value_(V), R_(Range::GetBottomRange()), Iterations_(0), Idx_(Idxs_++) {
}

Pointer::Pointer(Value *V, Range R)
  : Value_(V), R_(R), Iterations_(0), Idx_(Idxs_++) {
}

// print
void Pointer::print(raw_ostream &OS) const {
  Value *V = getValue();
  printProlog(OS);
  OS << "(";
  PrintValue(OS, V);
  OS << ")";
  ArgsTy::const_iterator J = begin(), E = end();
  if (J == E)
    return;
  OS << "[";
  for (; J != E; ++J) {
    if (J != begin())
      OS << " :: ";
    PrintValue(OS, (*J)->getValue());
  }
  OS << "]";
  printEpilog(OS);
}

// printAsDot
void Pointer::printAsDot(raw_ostream &OS) const {
  OS << "  \"" << getIdx() << "\"";
  OS << " [label = \"" << *this << "\", color = \"cyan2\"]\n";
  for (auto& J : *this)
  OS << "  \"" << J->getIdx() << "\" -> \"" << getIdx()
     << "\" [color = \"cyan2\"]\n";
}

// setRange
void Pointer::setRange(Range R) {
  R_ = R;
}

// pushArg
void Pointer::pushArg(Pointer *J) {
  Args_.push_back(J);
}

// getArg
Pointer *Pointer::getArg(unsigned Idx) const {
  return Args_[Idx];
}

// setArg
void Pointer::setArg(unsigned Idx, Pointer *J) {
  assert(Idx < getNumArgs() && "Invalid index specified");
  Args_[Idx] = J;
}

// hasArg
bool Pointer::hasArg(Pointer *J) const {
  return find(Args_.begin(), Args_.end(), J) != Args_.end();
}

// getNumArgs
size_t Pointer::getNumArgs() const {
  return Args_.size();
}

// subs
void Pointer::subs(DenseMap<Pointer*, Pointer*> Subs) {
  for (unsigned Idx = 0; Idx < getNumArgs(); ++Idx)
    if (Subs.count(getArg(Idx)))
      setArg(Idx, Subs[getArg(Idx)]);
}

unsigned Pointer::Idxs_ = 0;

/****************
 * CallPointer *
 ****************/
CallPointer::CallPointer(CallInst *CI, Range R, vector<pair<Range, Range> > Subs)
  : Pointer(CI, R) {
}

Range CallPointer::eval() {
  RG_DEBUG(dbgs() << "eval(): " << *this << "\n");
  return getRange();
  /* Function *F = cast<CallInst>(getValue())->getCalledFunction();
  if (!F || getNumArgs() == 0)
    return getRange();
  errs() << "CallInst: " << getValue() << "\n";
  Pointer *P = getArg(0);
  Range R = P->eval();
  //R.subs(Subs_);
  errs() << "Evals to: " << R << "\n";
  return R; */
}

/****************
 * BasePointer *
 ****************/
BasePointer::BasePointer(Value *V)
  : Pointer(V) {
}

BasePointer::BasePointer(Value *V, Range R)
  : Pointer(V, R) {
}

// eval
Range BasePointer::eval() {
  RG_DEBUG(dbgs() << "eval(): " << *this << "\n");
  return getRange();
}

/****************
 * NoopPointer *
 ****************/
NoopPointer::NoopPointer(Value *V)
  : Pointer(V), IsZero_(false) {
}

NoopPointer::NoopPointer(Value *V, Pointer *J)
  : Pointer(V), IsZero_(false) {
  pushArg(J);
}

void NoopPointer::setZero() {
  IsZero_ = true;
}

// eval
Range NoopPointer::eval() {
  RG_DEBUG(dbgs() << "eval(): " << *this << "\n");
  if (IsZero_) {
    setRange(Range(Expr::GetZeroValue()));
    return Range(Expr::GetZeroValue());
  }
  Range R = getArg(0)->eval();
  setRange(R);
  return R;
}

/****************
 * IndexPointer *
 ****************/
IndexPointer::IndexPointer(GetElementPtrInst *GEP, Pointer *Incoming, Range Index)
  : Pointer(GEP), Index_(Index) {
  pushArg(Incoming);
}

// eval
Range IndexPointer::eval() {
  Range R = getArg(0)->eval();
  R = Range(R.getLower() - Index_.getLower(), R.getUpper() - Index_.getUpper());
  setRange(R);
  return R;
}

/**************
 * PhiPointer *
 **************/
PhiPointer::PhiPointer(Value *V)
  : Pointer(V), Widened_(false) {
}

// pushIncoming
void PhiPointer::pushIncoming(Pointer *Incoming) {
  pushArg(Incoming);
}

// eval
Range PhiPointer::eval() {
  RG_DEBUG(dbgs() << "eval(): " << *this << "\n");

  if (isWidened()) {
    RG_DEBUG(dbgs() << "        isWidened()\n");
    return getRange();
  }

  bool ShouldStop   = getIterations() >= 2;
  bool ShouldWiden  = getIterations() == 0;

  RG_DEBUG(dbgs() << "        ShouldStop, ShouldWiden: " << ShouldStop
                  << ", " << ShouldWiden << "\n");

  // If we've already gone through all iterations, use the available
  // junction ranges instead of evaluating them again.
  // If we haven't completed the iterations, evaluate the arguments.
  incIterations();
  ArgsTy Args = getArgs();
  Range R;
  R = ShouldStop ? GetMeet(Args) : GetEvalMeet(Args);

  if (ShouldWiden) {
    R = R.widen(getRange());
    setWidened();
  }

  setRange(R);
  return R;
}

/******************
 * RegionAnalysis *
 ******************/
// getAnalysisUsage
void RegionAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTree>();
  AU.addRequired<DominanceFrontier>();
  AU.addRequired<TargetLibraryInfo>();
  AU.addRequired<DataLayout>();
  AU.addRequired<SymbolicRangeAnalysis>();
  AU.setPreservesAll();
}

// runOnModule
bool RegionAnalysis::runOnModule(Module& M) {
  auto Start = std::chrono::high_resolution_clock::now();

  bool OrRaDebug = RaDebug;
  bool OrRaDebugEval = RaDebugEval;

  Context = &M.getContext();

  TLI_ = &getAnalysis<TargetLibraryInfo>();
  DL_ = &getAnalysis<DataLayout>();

  // Insert the functions from LibMallocLikeFns, mapping each allocation
  // function to its first parameter.
  for (unsigned Idx = 0; Idx < array_lengthof(LibMallocLikeFns); ++Idx) {
    StringRef Name = TLI_->getName(LibMallocLikeFns[Idx]);
    if (Function *F = M.getFunction(Name)) {
      // Check the function type.
      FunctionType *FTy = F->getFunctionType();
      if (FTy->getReturnType() == Type::getInt8PtrTy(*Context) &&
          FTy->getNumParams() == 1 &&
          (FTy->getParamType(0)->isIntegerTy(32) ||
          FTy->getParamType(0)->isIntegerTy(64))) {
        Value *Arg = &(*F->arg_begin());
        ValueSizes_[F] = Range(Expr::GetZeroValue(), Expr(Arg));
      }
    }
  }

  // Do the same for LibFreeLikeFns, topping out the abstract state of the
  // first parameter.
  for (unsigned Idx = 0; Idx < array_lengthof(LibFreeLikeFns); ++Idx) {
    StringRef Name = TLI_->getName(LibFreeLikeFns[Idx]);
    if (Function *F = M.getFunction(Name)) {
      // Check the function type.
      FunctionType *FTy = F->getFunctionType();
      if (FTy->getReturnType()->isVoidTy() &&
          FTy->getNumParams() == 1 &&
          FTy->getParamType(0) == Type::getInt8PtrTy(*Context)) {
        Value *Arg = &(*F->arg_begin());
        ValueSizes_[Arg] = Range(Expr::GetZeroValue());
      }
    }
  }

  for (auto& G : M.getGlobalList())
    createPointerForGlobal(&G);

  // Create pointers for all arguments of every function.
  // This is necessary since we handle call instructions during analysis
  // of the caller.
  for (auto& F : M) {
    if (F.isIntrinsic() || F.isDeclaration())
      continue;

    if (!RaDebugFunction.empty()) {
      if (F.getName() != RaDebugFunction) {
        RaDebug = false;
        RaDebugEval = false;
      } else {
        RaDebug = true;
        RaDebugEval = true;
      }
    }

    createPointersForFunctionArgs(&F);

    /* if (F.getFunctionType()->isPointerTy())
      for (auto& BB : F) {
        TerminatorInst *TI = BB.getTerminator();
        if (ReturnInst *RI = dyn_cast<ReturnInst>(TI)) {
          ReturnVal_[&F] = createNoopPointer(RI);
          break;
        }
      } */
  }

  RG_ = &getAnalysis<SymbolicRangeAnalysis>();

  for (auto& F : M) {
    if (F.isIntrinsic() || F.isDeclaration())
      continue;

    if (!RaDebugFunction.empty()) {
      if (F.getName() != RaDebugFunction) {
        RaDebug = false;
        RaDebugEval = false;
      } else {
        RaDebug = true;
        RaDebugEval = true;
      }
    }

    DT_ = &getAnalysis<DominatorTree>(F);
    DF_ = &getAnalysis<DominanceFrontier>(F);

    createConstraintsForFunction(&F);
  }

  RaDebug = OrRaDebug;
  RaDebugEval = OrRaDebugEval;

  auto End = std::chrono::high_resolution_clock::now();
  auto Elapsed = std::chrono::duration_cast<std::chrono::milliseconds>
                   (End - Start).count();

  dbgs() << "================== RA STATS =================\n";
  dbgs() << "==== Total evaluation time:     " << Elapsed << "\t ====\n";
  dbgs() << "==== Number of pointers:        " << PointersMap_.size() << "\n";

  if (RaStats) {
    dbgs() << "================== RA STATS =================\n";
    dbgs() << "==== Number of pointers:       " << PointersMap_.size() << "\n";
  }

  if (RaDebug)
    dbgs() << *this;

  return false;
}

void RegionAnalysis::print(raw_ostream& OS) const {
  vector<pair<Pointer*, Range> > Ranges;
  for (auto& J : PointersMap_) {
    if (!RaDebugFunction.empty()) {
      if (Instruction *I = dyn_cast<Instruction>(J.first)) {
        if (I->getParent()->getParent()->getName() != RaDebugFunction)
          continue;
      } else if (Argument *A = dyn_cast<Argument>(J.first)) {
        if (A->getParent()->getName() != RaDebugFunction)
          continue;
      } else if (isa<Constant>(J.first)) {
        continue;
      }
    }

    RG_DEBUG(dbgs() << "==== eval() ==== " << *J.second << " ====\n");
    Ranges.push_back(make_pair(J.second, J.second->eval()));
  }

  OS << "================== POINTERS =================\n";
  for (auto& P : Ranges) {
    OS << *P.first<< " = " << P.second << "\n";
  }
}

// printDot
void RegionAnalysis::printAsDot(raw_ostream& OS) const {
  OS << "digraph module {\n";
  OS << "  graph [rankdir = LR, margin = 0];\n";
  OS << "  node [shape = record,fontname = \"Times-Roman\", fontsize = 14];\n";
  for (auto& J : PointersMap_)
    J.second->printAsDot(OS);
  OS << "}\n";
}

// printInfo
void RegionAnalysis::printInfo(raw_ostream& OS) const {
  OS << "## Function:    " << F_->getName()  << "\n";
  OS << "## Block:       " << BB_->getName() << "\n";
  OS << "## Instruction: " << I_->getName()  << "\n";
}

// getRange
Range RegionAnalysis::getRange(Value *V) {
  if (PointersMap_.count(V))
    return PointersMap_[V]->eval();
  return Range(Expr::GetPlusInfValue(), Expr::GetMinusInfValue());
}

// setPointer
void RegionAnalysis::setPointer(Value *V, Pointer *J) {
  PointersMap_[V] = J;
}

// getPointerForValue
Pointer *RegionAnalysis::getPointerForValue(Value *V) {
  assert(V->getType()->isPointerTy() && "Invalid non-integer value");
  auto It = PointersMap_.find(V);
  if (It == PointersMap_.end()) {
    ASSERT_TYPE(V, Constant, "Uninitialized value is not a constant");
    return createBasePointer(V);
  }
  return It->second;
}

// createConstraintsForFunction
void RegionAnalysis::createConstraintsForFunction(Function *F) {
  F_ = F;

  for (auto& BB : *F) {
    BB_ = &BB;
    for (auto& I : BB)
      if (isa<PHINode>(&I))
        createPointerForPhiNode(cast<PHINode>(&I));
  }

  for (auto& BB : *F) {
    BB_ = &BB;
    for (auto& I : BB)
      createPointerForInst(&I);
  }

  for (auto& BB : *F) {

    /* TerminatorInst *TI = BB.getTerminator();
    if (ReturnInst *RI = dyn_cast<ReturnInst>(TI))
      if (ReturnVal_.count(F))
        ReturnVal_[F]->pushArg(getPointerForValue(RI->getReturnValue())); */

    for (auto& I : BB)
      createConstraintsForInst(&I);
  }
}

// createPointersForFunction
void RegionAnalysis::createPointersForFunctionArgs(Function *F) {
  F_ = F;

  // Create phi pointers for the funcion operands.
  // TODO: Correctly handle recursive functions.
  for (auto AI = F->arg_begin(), AE = F->arg_end(); AI != AE; ++AI) {
    if (!AI->getType()->isPointerTy() || AI->getType()->isVoidTy())
      continue;
    createPhiPointer(&(*AI));
  }
}

// createPointerForGlobal
Pointer *RegionAnalysis::createPointerForGlobal(GlobalValue *GV) {
  Type *T = GV->getType();
  if (T->isPointerTy())
    T = T->getPointerElementType();
  if (T->isVoidTy() || T->isFunctionTy())
    return NULL;
  return createPointerForType(GV, T);
}

// createConstraintsForInst
void RegionAnalysis::createConstraintsForInst(Instruction *I) {
  RG_DEBUG(dbgs() << "createConstraintsForInst: " << *I << "\n");
  //if (!I->getType()->isPointerTy())
  //  return;
  I_ = I;
  unsigned Opcode = I->getOpcode();
  switch (Opcode) {
    case Instruction::PHI:
      if (I->getType()->isPointerTy())
        createConstraintsForPhi(cast<PHINode>(I));
      break;
    case Instruction::Call:
      createConstraintsForCall(cast<CallInst>(I));
      break;
    default:
      break;
  }
}

// createConstraintsForPhi
void RegionAnalysis::createConstraintsForPhi(PHINode *Phi) {
  RG_DEBUG(dbgs() << "createConstraintsForPhi: " << *Phi << "\n");
  if (!Phi->getType()->isPointerTy())
    return;
  PhiPointer *J = (PhiPointer*)(getPointerForValue(Phi));
  for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx) {
    Pointer *Incoming = getPointerForValue(Phi->getIncomingValue(Idx));
    J->pushIncoming(Incoming);
  }
}
    
// createConstraintsForCall
void RegionAnalysis::createConstraintsForCall(CallInst *CI) {
  if (Function *F = CI->getCalledFunction()) {
    if (F->isIntrinsic() || F->isDeclaration())
      return;
    //errs() << "F: " << F->getName() << "\n";
    for (auto AI = F->arg_begin(), AE = F->arg_end(); AI != AE; ++AI) {
      if (!AI->getType()->isPointerTy() || AI->getType()->isVoidTy())
        continue;
      PhiPointer *P = cast<PhiPointer>(getPointerForValue(&(*AI)));
      //if (!P)
      //  continue;
      Value *Real = CI->getArgOperand(AI->getArgNo());
      //errs() << "Pushing: " << *Real << " for " << *AI << "\n";
      P->pushIncoming(getPointerForValue(Real));
    }
  }
}

// createPointerForInst
void RegionAnalysis::createPointerForInst(Instruction *I) {
  if (!I->getType()->isPointerTy())
    return;
  RG_DEBUG(dbgs() << "createPointerForInst: " << *I << "\n");
  unsigned Predicate = I->getOpcode();
  switch (Predicate) {
    case Instruction::Alloca:
      createPointerForAlloca(cast<AllocaInst>(I));
      break;
    case Instruction::Call:
      createPointerForCall(cast<CallInst>(I));
      break;
    case Instruction::BitCast:
      createPointerForBitCast(cast<BitCastInst>(I));
      break;
    case Instruction::GetElementPtr:
      createPointerForGetElementPtr(cast<GetElementPtrInst>(I));
      break;
    //case Instruction::PHI:
    //  createPointerForPhiNode(cast<PHINode>(I));
    //  break;
    default:
      createBasePointer(I);
  }
}

void RegionAnalysis::createPointerForAlloca(AllocaInst *AI) {
  Type *T = AI->getAllocatedType();
  createPointerForType(AI, T);
}

void RegionAnalysis::createPointerForGetElementPtr(GetElementPtrInst *GEP) {
  createIndexPointer(GEP);
}

void RegionAnalysis::createPointerForCall(CallInst *CI) {
  if (Function *F = CI->getCalledFunction()) {
    //errs() << "createCallPointer: " << *CI << "\n";
    createCallPointer(CI, F);
  } else {
    //errs() << "createBasePointer: " << *CI << "\n";
    createBasePointer(CI);
  }
}

void RegionAnalysis::createPointerForBitCast(BitCastInst *BCI) {
  if (BCI->getOperand(0)->getType()->isPointerTy())
    createNoopPointer(BCI, BCI->getOperand(0));
  else
    createBasePointer(BCI);
}

void RegionAnalysis::createPointerForPhiNode(PHINode *Phi) {
  createPhiPointer(Phi);
}

// createBasePointer
BasePointer *RegionAnalysis::createBasePointer(Value *V) {
  RG_DEBUG(dbgs() << "createBasePointer: " << *V << "\n");
  BasePointer *J = new BasePointer(V, Range(Expr::GetPlusInfValue(), Expr::GetMinusInfValue()));
  setPointer(V, J);
  return J;
}

// createBasePointer
BasePointer *RegionAnalysis::createBasePointer(Value *V, Range R) {
  RG_DEBUG(dbgs() << "createBasePointer: " << *V << ", " << R << "\n");
  BasePointer *J = new BasePointer(V, R);
  setPointer(V, J);
  return J;
}

Range RegionAnalysis::getRangeForFunction(Function *F) {
  return ValueSizes_[F];
}

Range RegionAnalysis::getRangeForCall(CallInst *CI) {
  Function *F = CI->getCalledFunction();
  Range R = getRangeForFunction(F);
  if (!F || !ValueSizes_.count(F) || F->isVarArg())
    return Range::GetInfRange();
  if (ValueSizes_[F] == Range::GetZeroRange()) {
    BasicBlock *BB = CI->getParent();
    auto I = BB->rbegin();
    while (&(*I) != CI)
      ++I;
    while (isa<BitCastInst>(&(*I)))
      cast<NoopPointer>(getPointerForValue(&(*I)))->setZero();
  }
  //assert(F && "Indirect are not handled");
  RG_DEBUG(dbgs() << "Range for " << F->getName() << " is " << R << "\n");
  vector<pair<Expr, Expr> > SubsLower, SubsUpper;
  unsigned Idx = 0;
  for (auto& AI : F->getArgumentList()) {
    RG_DEBUG(dbgs() << "Replace " << Expr(&AI) << " with " << Expr(CI->getArgOperand(Idx)) << "\n");
    Range ArgR = RG_->getRange(CI->getArgOperand(Idx++));
    SubsLower.push_back(make_pair(Expr(&AI), ArgR.getLower()));//Expr(CI->getArgOperand(Idx++))));
    SubsUpper.push_back(make_pair(Expr(&AI), ArgR.getUpper()));//Expr(CI->getArgOperand(Idx++))));
  }
  R = Range(R.getLower().subs(SubsLower), R.getUpper().subs(SubsUpper));
  RG_DEBUG(dbgs() << "Subs for " << *CI << " is " << R << "\n");
  return R;
}

// createPhiPointer
PhiPointer *RegionAnalysis::createPhiPointer(Value *V) {
  RG_DEBUG(dbgs() << "createPhiPointer: " << *V << "\n");
  PhiPointer *J = new PhiPointer(V);
  setPointer(V, J);
  return J;
}

// createCallPointer
Pointer *RegionAnalysis::createCallPointer(CallInst *CI, Function *F) {
  Range R = getRangeForCall(CI);
  return createBasePointer(CI, R);
  // FIXME:
  // Handle indirect calls.
  //if (!F)
  //  return NULL;
  /* unsigned Idx = 0;
  vector<pair<Range, Range> > Subs;
  for (auto A = F->arg_begin(); A != F->arg_end(); ++A) {
    Value *Real = CI->getOperand(Idx++);
    if (Real->getType()->isIntegerTy())
      continue;
    Range RR = RG_->getRange(Real);
    Range FR = RG_->getRange(&(*A));
    //if (FR.isSymbol())
    //  Subs.push_back(make_pair(FR, RR));
  }
  if (ValueSizes_.count(F)) {
    Range R = getRangeForCall(CI);
    return createBasePointer(CI, R);
  }
  return createBasePointer(CI);
  return NULL;
  RG_DEBUG(dbgs() << "createCallPointer: " << *CI << "\n");
  //Range R = getRangeForCall(CI);
  CallPointer *J = new CallPointer(CI, Range::GetInfRange(), Subs);
  assert(ReturnVal_[F]);
  J->pushArg(ReturnVal_[F]);
  setPointer(CI, J);
  //return J; */
}

// createNoopPointer
NoopPointer *RegionAnalysis::createNoopPointer(Value *V, Value *Op) {
  RG_DEBUG(dbgs() << "createNoopPointer: " << *V << " :: " << *Op << "\n");

  Pointer *Incoming = getPointerForValue(Op);
  NoopPointer *J = new NoopPointer(V, Incoming);
  setPointer(V, J);
  return J;
}

// createNoopPointer
NoopPointer *RegionAnalysis::createNoopPointer(Value *V) {
  RG_DEBUG(dbgs() << "createNoopPointer: " << *V << "\n");
  NoopPointer *J = new NoopPointer(V);
  setPointer(V, J);
  return J;
}


// createIndexPointer
IndexPointer *RegionAnalysis::createIndexPointer(GetElementPtrInst *GEP) {
  RG_DEBUG(dbgs() << "createIndexPointer: " << *GEP << "\n");
  Pointer *Incoming = getPointerForValue(GEP->getPointerOperand());
  
  Expr Lower;
  Expr Upper;

  Value *Pointer = GEP->getPointerOperand();
  Type *PointerTy = Pointer->getType();

  for (unsigned Idx = 1; Idx < GEP->getNumOperands(); ++Idx) {
    if (StructType *ST = dyn_cast<StructType>(PointerTy)) {
      ConstantInt *CI = cast<ConstantInt>(GEP->getOperand(Idx));
      APInt Int = CI->getValue();
      uint64_t Val = Int.getZExtValue();
      unsigned Size = 0;
      for (uint64_t SIdx = 0; SIdx < Val; ++SIdx) {
        Type *T = ST->getElementType(SIdx);
        Size += DL_->getTypeAllocSize(T);
      }
      Lower = Lower + Size; 
      Upper = Upper + Size;
      continue;
    }

    if (PointerType *PT = dyn_cast<PointerType>(PointerTy))
      PointerTy = PT->getElementType();
    else if (ArrayType *AT = dyn_cast<ArrayType>(PointerTy))
      PointerTy = AT->getElementType();


    unsigned AllocSize = DL_->getTypeAllocSize(PointerTy);

    RG_DEBUG(dbgs() << "PointerTy: " << *PointerTy << ", " << AllocSize << "\n");

    Lower = Lower + RG_->getRange(GEP->getOperand(Idx)).getLower() * Expr(AllocSize);
    Upper = Upper + RG_->getRange(GEP->getOperand(Idx)).getUpper() * Expr(AllocSize);
  }

  Range R(Lower, Upper);
  RG_DEBUG(dbgs() << "Index: " << R << "\n");
  IndexPointer *J = new IndexPointer(GEP, Incoming, R);
  setPointer(GEP, J);
  return J;
}

BasePointer *RegionAnalysis::createPointerForType(Value *V, Type *T) {
  RG_DEBUG(dbgs() << "createPointerForType: " << *V << " :: " << *T << "\n");
  if (StructType *ST = dyn_cast<StructType>(T)) {
    Expr Upper(DL_->getTypeAllocSize(ST));
    //for (auto TI = ST->subtye_begin(), TE = ST->subtype_end();
    //     TI != TE; ++TI) {
    //}
    return createBasePointer(V, Range(Expr::GetZeroValue(), Upper)); 
  } else if (ArrayType *AT = dyn_cast<ArrayType>(T)) {
    unsigned AllocSize = DL_->getTypeAllocSize(AT->getElementType());
    Expr Upper(AT->getNumElements() * AllocSize);
    return createBasePointer(V, Range(Expr::GetZeroValue(), Upper)); 
  }
  unsigned AllocSize = DL_->getTypeAllocSize(T);
  return createBasePointer(V, Range(Expr::GetZeroValue(), Expr(AllocSize)));
}

/*****************
 * AnnotateLoads *
 *****************/
class AnnotateLoads : public ModulePass {
public:
  static char ID;
  AnnotateLoads() : ModulePass(ID) { }

  virtual bool runOnModule(Module& M);

private:
  void setMetadataOn(Instruction *I); 

  void visitCall(CallInst *CI, Value *V);
  void visitGetElementPtr(GetElementPtrInst *GEP, Value *V);
  void visitBitCast(BitCastInst *BCI, Value *V); 
  void visitPhiNode(PHINode *Phi, Value *V); 
  void visitStore(StoreInst *SI, Value *V);
  void visitLoad(LoadInst *LI, Value *V);

  LLVMContext *Context_;

  queue<Value*> Loads_;
  set<Value*>   Visited_;
  set<Value*>   InitDeps_;
  set<Value*>   Deps_;

  unsigned LidepStores_;
  unsigned TotalStores_;
  unsigned LidepLoads_;
  unsigned TotalLoads_;
};

bool AnnotateLoads::runOnModule(Module &M) {
  Context_ = &M.getContext();

  LidepStores_  = 0;
  TotalStores_ = 0;
  LidepLoads_   = 0;
  TotalLoads_  = 0;

  for (auto& F : M)
    for (auto& BB : F)
      for (auto& I : BB)
        if (isa<LoadInst>(&I))
          Loads_.push(&I);

  while (!Loads_.empty()) {
    Value *V = Loads_.front(); Loads_.pop();

    if (Visited_.count(V))
      continue;
    Visited_.insert(V);

    for (auto U = V->use_begin(); U != V->use_end(); ++U) {
      Instruction *I = dyn_cast<Instruction>(*U);
      if (!I)
        continue;
      switch (I->getOpcode()) {
        case Instruction::Call:
          visitCall(cast<CallInst>(I), V);
          break;
        case Instruction::GetElementPtr:
          visitGetElementPtr(cast<GetElementPtrInst>(I), V);
          break;
        case Instruction::BitCast:
          visitBitCast(cast<BitCastInst>(I), V);
          break;
        case Instruction::PHI:
          visitPhiNode(cast<PHINode>(I), V);
          break;
        case Instruction::Store:
          visitStore(cast<StoreInst>(I), V);
          break;
        case Instruction::Load:
          visitLoad(cast<LoadInst>(I), V);
          break;
      }
    }
  }

  float PercentageLidepLoadsNum = (float)LidepLoads_/(float)TotalLoads_;
  ostringstream PercentageLidepLoadsStream; 
  PercentageLidepLoadsStream << setprecision(4) << PercentageLidepLoadsNum;
  string PercentageLidepLoads = PercentageLidepLoadsStream.str(); 

  float PercentageLidepStoresNum = (float)LidepStores_/(float)TotalStores_;
  ostringstream PercentageLidepStoresStream; 
  PercentageLidepStoresStream << setprecision(4) << PercentageLidepStoresNum;
  string PercentageLidepStores = PercentageLidepStoresStream.str(); 

  float PercentageTotalLidepNum = (float)(LidepLoads_  + LidepStores_)/
                                  (float)(TotalLoads_ + TotalStores_);
  ostringstream PercentageTotalLidepStream; 
  PercentageTotalLidepStream << setprecision(4) << PercentageTotalLidepNum;
  string PercentageTotalLidep = PercentageTotalLidepStream.str(); 

  dbgs() << "================= LIDEP STATS ===============\n";
  // dbgs() << "==== Number of visited loads:   "  << TotalLoads_          << "\n";
  dbgs() << "==== Number of load-dep loads:  "  << LidepLoads_           << "\n";
  // dbgs() << "==== \% of load-dep loads:       " << PercentageLidepLoads  << "\n";
  // dbgs() << "==== Number of visited stores:  "  << TotalStores_         << "\n";
  dbgs() << "==== Number of load-dep stores: "  << LidepStores_          << "\n";
  // dbgs() << "==== \% of load-dep stores:      " << PercentageLidepStores << "\n";
  // dbgs() << "==== \% of load-dep accesses:    " << PercentageTotalLidep  << "\n";

  return false;
}

void AnnotateLoads::setMetadataOn(Instruction *I) {
  I->setMetadata("li_dep", MDNode::get(*Context_,  ArrayRef<Value*>()));
}

void AnnotateLoads::visitCall(CallInst *CI, Value *V) {
  if (Function *F = CI->getCalledFunction()) {
    unsigned Idx = 0;
    for (auto A = F->arg_begin(); A != F->arg_end(); ++A) {
      if (CI->getOperand(Idx++) == V)
        Loads_.push(&(*A));
      if (Deps_.count(V))
        Deps_.insert(&(*A));
    }
  }
}

void AnnotateLoads::visitGetElementPtr(GetElementPtrInst *GEP, Value *V) {
  if (GEP->getOperand(0) == V) {
    setMetadataOn(GEP);
    Loads_.push(GEP);
    if (Deps_.count(V) || isa<LoadInst>(V))
      Deps_.insert(GEP);
  }
}

void AnnotateLoads::visitBitCast(BitCastInst *BCI, Value *V) {
  if (BCI->getOperand(0) == V) {
    setMetadataOn(BCI);
    Loads_.push(BCI);
    if (Deps_.count(V) || isa<LoadInst>(V))
      Deps_.insert(BCI);
  }
}

void AnnotateLoads::visitPhiNode(PHINode *Phi, Value *V) {
  setMetadataOn(Phi);
  Loads_.push(Phi);
  if (Deps_.count(V) || isa<LoadInst>(V))
    Deps_.insert(Phi);
}

void AnnotateLoads::visitStore(StoreInst *SI, Value *V) {
  TotalStores_++;
  if (SI->getPointerOperand() == V) {
    setMetadataOn(SI);
    if (Deps_.count(V) || isa<LoadInst>(V)) {
      Deps_.insert(SI);
      LidepStores_++;
    }
  }
}

void AnnotateLoads::visitLoad(LoadInst *LI, Value *V) {
  TotalLoads_++;
  if (LI->getPointerOperand() == V) {
    setMetadataOn(LI);
    Loads_.push(LI);
    if (Deps_.count(V) || isa<LoadInst>(V)) {
      Deps_.insert(LI);
      LidepLoads_++;
    }
  }
}

char AnnotateLoads::ID = 0;
static RegisterPass<AnnotateLoads>
  P("region-analysis-annotate-loads", "Annotate load-dependant values",
    true, true);

/*****************
 * AnnotateCalls *
 *****************/
class AnnotateCalls : public ModulePass {
public:
  static char ID;
  AnnotateCalls() : ModulePass(ID) { }

  virtual bool runOnModule(Module& M);

private:
  void setMetadataOn(Instruction *I); 

  void visitCall(CallInst *CI, Value *V);
  void visitGetElementPtr(GetElementPtrInst *GEP, Value *V);
  void visitBitCast(BitCastInst *BCI, Value *V); 
  void visitPhiNode(PHINode *Phi, Value *V); 
  void visitStore(StoreInst *SI, Value *V);
  void visitLoad(LoadInst *LI, Value *V);

  LLVMContext *Context_;

  queue<Value*> Calls_;
  set<Value*>   Visited_;
  set<Value*>   InitDeps_;
  set<Value*>   Deps_;

  unsigned CidepStores_;
  unsigned TotalStores_;
  unsigned CidepLoads_;
  unsigned TotalLoads_;
};

bool AnnotateCalls::runOnModule(Module &M) {
  Context_ = &M.getContext();

  CidepStores_  = 0;
  TotalStores_ = 0;
  CidepLoads_   = 0;
  TotalLoads_  = 0;

  for (auto& F : M)
    for (auto& BB : F)
      for (auto& I : BB)
        if (isa<CallInst>(&I))
          Calls_.push(&I);

  while (!Calls_.empty()) {
    Value *V = Calls_.front(); Calls_.pop();

    if (Visited_.count(V))
      continue;
    Visited_.insert(V);

    for (auto U = V->use_begin(); U != V->use_end(); ++U) {
      Instruction *I = dyn_cast<Instruction>(*U);
      if (!I)
        continue;
      switch (I->getOpcode()) {
        case Instruction::Call:
          visitCall(cast<CallInst>(I), V);
          break;
        case Instruction::GetElementPtr:
          visitGetElementPtr(cast<GetElementPtrInst>(I), V);
          break;
        case Instruction::BitCast:
          visitBitCast(cast<BitCastInst>(I), V);
          break;
        case Instruction::PHI:
          visitPhiNode(cast<PHINode>(I), V);
          break;
        case Instruction::Store:
          visitStore(cast<StoreInst>(I), V);
          break;
        case Instruction::Load:
          visitLoad(cast<LoadInst>(I), V);
          break;
      }
    }
  }

  float PercentageCidepLoadsNum = (float)CidepLoads_/(float)TotalLoads_;
  ostringstream PercentageCidepLoadsStream; 
  PercentageCidepLoadsStream << setprecision(4) << PercentageCidepLoadsNum;
  string PercentageCidepLoads = PercentageCidepLoadsStream.str(); 

  float PercentageCidepStoresNum = (float)CidepStores_/(float)TotalStores_;
  ostringstream PercentageCidepStoresStream; 
  PercentageCidepStoresStream << setprecision(4) << PercentageCidepStoresNum;
  string PercentageCidepStores = PercentageCidepStoresStream.str(); 

  float PercentageTotalCidepNum = (float)(CidepLoads_  + CidepStores_)/
                                  (float)(TotalLoads_ + TotalStores_);
  ostringstream PercentageTotalCidepStream; 
  PercentageTotalCidepStream << setprecision(4) << PercentageTotalCidepNum;
  string PercentageTotalCidep = PercentageTotalCidepStream.str(); 

  dbgs() << "================= CIDEP STATS ===============\n";
  // dbgs() << "==== Number of visited loads:   "  << TotalLoads_          << "\n";
  dbgs() << "==== Number of call-dep loads:  "  << CidepLoads_           << "\n";
  // dbgs() << "==== \% of load-dep loads:       " << PercentageLidepLoads  << "\n";
  // dbgs() << "==== Number of visited stores:  "  << TotalStores_         << "\n";
  dbgs() << "==== Number of call-dep stores: "  << CidepStores_          << "\n";
  // dbgs() << "==== \% of load-dep stores:      " << PercentageLidepStores << "\n";
  // dbgs() << "==== \% of load-dep accesses:    " << PercentageTotalLidep  << "\n";

  return false;
}

void AnnotateCalls::setMetadataOn(Instruction *I) {
  I->setMetadata("ci_dep", MDNode::get(*Context_,  ArrayRef<Value*>()));
}

void AnnotateCalls::visitCall(CallInst *CI, Value *V) {
  if (Function *F = CI->getCalledFunction()) {
    unsigned Idx = 0;
    for (auto A = F->arg_begin(); A != F->arg_end(); ++A) {
      if (CI->getOperand(Idx++) == V)
        Calls_.push(&(*A));
      if (Deps_.count(V))
        Deps_.insert(&(*A));
    }
  }
}

void AnnotateCalls::visitGetElementPtr(GetElementPtrInst *GEP, Value *V) {
  if (GEP->getOperand(0) == V) {
    setMetadataOn(GEP);
    Calls_.push(GEP);
    if (Deps_.count(V) || isa<CallInst>(V))
      Deps_.insert(GEP);
  }
}

void AnnotateCalls::visitBitCast(BitCastInst *BCI, Value *V) {
  if (BCI->getOperand(0) == V) {
    setMetadataOn(BCI);
    Calls_.push(BCI);
    if (Deps_.count(V) || isa<CallInst>(V))
      Deps_.insert(BCI);
  }
}

void AnnotateCalls::visitPhiNode(PHINode *Phi, Value *V) {
  setMetadataOn(Phi);
  Calls_.push(Phi);
  if (Deps_.count(V) || isa<CallInst>(V))
    Deps_.insert(Phi);
}

void AnnotateCalls::visitStore(StoreInst *SI, Value *V) {
  TotalStores_++;
  if (SI->getPointerOperand() == V) {
    setMetadataOn(SI);
    if (Deps_.count(V) || isa<CallInst>(V)) {
      Deps_.insert(SI);
      CidepStores_++;
    }
  }
}

void AnnotateCalls::visitLoad(LoadInst *LI, Value *V) {
  TotalLoads_++;
  if (LI->getPointerOperand() == V) {
    setMetadataOn(LI);
    Calls_.push(LI);
    if (Deps_.count(V) || isa<CallInst>(V)) {
      Deps_.insert(LI);
      CidepLoads_++;
    }
  }
}

char AnnotateCalls::ID = 0;
static RegisterPass<AnnotateCalls>
  Q("region-analysis-annotate-calls", "Annotate call-dependant values",
    true, true);

/******************
 * AnnotateSafety *
 ******************/
class AnnotateSafety : public ModulePass {
public:
  static char ID;
  AnnotateSafety() : ModulePass(ID) { }

  virtual void getAnalysisUsage(AnalysisUsage& AU) const;
  virtual bool runOnModule(Module& M);

private:
  void setMetadataOn(Instruction *I); 

  void visitStore(StoreInst *SI);
  void visitLoad(LoadInst *LI);

  LLVMContext *Context_;
  RegionAnalysis *RA_;

  unsigned SafeStores_;
  unsigned TotalStores_;
  unsigned SafeLoads_;
  unsigned TotalLoads_;
};

void AnnotateSafety::getAnalysisUsage(AnalysisUsage& AU) const {
  AU.addRequired<RegionAnalysis>();
  AU.setPreservesAll();
}

bool AnnotateSafety::runOnModule(Module& M) {
  Context_ = &M.getContext();

  RA_ = &getAnalysis<RegionAnalysis>();

  SafeStores_  = 0;
  TotalStores_ = 0;
  SafeLoads_   = 0;
  TotalLoads_  = 0;

  for (auto& F : M)
    for (auto& BB : F)
      for (auto& I : BB)
        switch (I.getOpcode()) {
          case Instruction::Load:
            visitLoad(cast<LoadInst>(&I));
            break;
          case Instruction::Store:
            visitStore(cast<StoreInst>(&I));
            break;
        }

  float PercentageSafeLoadsNum = (float)SafeLoads_/(float)TotalLoads_;
  ostringstream PercentageSafeLoadsStream; 
  PercentageSafeLoadsStream << setprecision(4) << PercentageSafeLoadsNum;
  string PercentageSafeLoads = PercentageSafeLoadsStream.str(); 

  float PercentageSafeStoresNum = (float)SafeStores_/(float)TotalStores_;
  ostringstream PercentageSafeStoresStream; 
  PercentageSafeStoresStream << setprecision(4) << PercentageSafeStoresNum;
  string PercentageSafeStores = PercentageSafeStoresStream.str(); 

  float PercentageTotalSafeNum = (float)(SafeLoads_  + SafeStores_)/
                                 (float)(TotalLoads_ + TotalStores_);
  ostringstream PercentageTotalSafeStream; 
  PercentageTotalSafeStream << setprecision(4) << PercentageTotalSafeNum;
  string PercentageTotalSafe = PercentageTotalSafeStream.str(); 

  dbgs() << "================ SAFETY STATS ===============\n";
  dbgs() << "==== Number of loads:          "  << TotalLoads_          << "\n";
  dbgs() << "==== Number of safe loads:     "  << SafeLoads_           << "\n";
  dbgs() << "==== \% of safe loads:          " << PercentageSafeLoads  << "\n";
  dbgs() << "==== Number of stores:         "  << TotalStores_         << "\n";
  dbgs() << "==== Number of safe stores:    "  << SafeStores_          << "\n";
  dbgs() << "==== \% of safe stores:         " << PercentageSafeStores << "\n";
  dbgs() << "==== \% of safe accesses:       " << PercentageTotalSafe  << "\n";

  return false;
}

void AnnotateSafety::setMetadataOn(Instruction *I) { 
  I->setMetadata("memsafe", MDNode::get(*Context_,  ArrayRef<Value*>()));
}

void AnnotateSafety::visitStore(StoreInst *SI) {
  TotalStores_++;
  Value *Ptr = SI->getPointerOperand();
  if (isa<Constant>(Ptr)) {
    SafeStores_++;
    setMetadataOn(SI);
  } else {
    Range R = RA_->getRange(Ptr);
    bool Lower = R.getLower().isNotPositive();
    bool Upper = R.getUpper().isNonNegative();
    if (Lower && Upper) {
      SafeStores_++;
      setMetadataOn(SI);
    }
  }
}

void AnnotateSafety::visitLoad(LoadInst *LI) {
  TotalLoads_++;
  Value *Ptr = LI->getPointerOperand();
  if (isa<Constant>(Ptr)) {
    SafeLoads_++;
    setMetadataOn(LI);
  } else {
    Range R = RA_->getRange(Ptr);
    bool Lower = R.getLower().isNotPositive();
    bool Upper = R.getUpper().isNonNegative();
    if (Lower && Upper) {
      SafeLoads_++;
      setMetadataOn(LI);
    }
  }
}

char AnnotateSafety::ID = 0;
static RegisterPass<AnnotateSafety>
  R("region-analysis-annotate-safety", "Annotate safe-to-dereference values",
    true, true);

/*****************
 * AnnotateSizes *
 *****************/
class AnnotateSizes : public ModulePass {
public:
  static char ID;
  AnnotateSizes() : ModulePass(ID) { }

  virtual void getAnalysisUsage(AnalysisUsage& AU) const;
  virtual bool runOnModule(Module& M);

private:
  void setMetadataOn(Instruction *I, Range R); 

  LLVMContext *Context_;

  RegionAnalysis *RA_;
};

void AnnotateSizes::getAnalysisUsage(AnalysisUsage& AU) const {
  AU.addRequired<RegionAnalysis>();
  AU.setPreservesAll();
}

bool AnnotateSizes::runOnModule(Module& M) {
  Context_ = &M.getContext();

  RA_ = &getAnalysis<RegionAnalysis>();

  for (auto& P : *RA_)
    if (Instruction *I = dyn_cast<Instruction>(P.first))
      setMetadataOn(I, P.second->eval());

  return false;
}

void AnnotateSizes::setMetadataOn(Instruction *I, Range R) { 
  string Str = string("[") + R.getLower().getStringRepr() + ", " +
               R.getUpper().getStringRepr() + "]";
  MDString *MDStr = MDString::get(*Context_, Str);
  I->setMetadata("size", MDNode::get(*Context_, ArrayRef<Value*>(MDStr)));
}

char AnnotateSizes::ID = 0;
static RegisterPass<AnnotateSizes>
  S("region-analysis-annotate-sizes", "Annotate pointer sizes infered by the "
    "region analysis", true, true);

//////////////////////////////////////
  //print(dbgs());
  //if (RaDebug) {
 //   print(dbgs());
    /* std::string ErrorMsg;
    Twine Name = Twine("region_analysis.") + F.getName() + ".dot";
    raw_fd_ostream DotGraphStream(Name.str().c_str(), ErrorMsg);
    if (ErrorMsg.empty()) {
      errs() << "Printing graph to cfg.dot\n";
      printAsDot(DotGraphStream);
    } else {
      errs() << "Unable to open cfg.dot: " << ErrorMsg << "\n";
    } */
  //}

// print
//void RegionAnalysis::print(raw_ostream& OS) const {
  /* DenseMap<Pointer*, Range> Ranges;

  for (auto& J : PointersMap_) {
    Ranges.insert(make_pair(J.second, J.second->eval()));
  }

  int Safe = 0, NotSafe = 0;
  for (auto& P : Ranges) {
    errs() << "P: " << *P.first->getValue() << " ==> " << P.second << "\n";
    bool Lower = P.second.getLower().isNotPositive();
    bool Upper = P.second.getUpper().isStrictlyPositive();
    if (Lower && Upper) {
      if (Instruction *I = dyn_cast<Instruction>(P.first->getValue())) {
        ++Safe;
        I->setMetadata("safe", MDNode::get(*Context,  ArrayRef<Value*>()));
        errs() << *I << " is safe\n";
      }
    } else {
      errs() << "Lower, Upper: " << P.second.getLower() << ", " << P.second.getUpper() << "\n";
      errs() << *P.first->getValue() << " is not safe\n";
      ++NotSafe;
    }
  }

  errs() << "Safe, NotSafe: " << Safe << ", " << NotSafe << "\n"; */

  //OS << "==== ==== ==== ==== POINTERS ==== ==== ==== ====\n";
  //for (auto& J : PointersMap_)
  //  OS << *J.second << "\n";

  //OS << "==== ==== ==== ==== RANGES ==== ==== ==== ====\n";
  //DenseMap<Pointer*, Range> Ranges;

  //for (auto& J : PointersMap_) {
  //  RG_DEBUG(dbgs() << "eval()ing " << *J.second << "\n");
  //  Ranges.insert(std::make_pair(J.second, J.second->eval()));
  //}

  //RG_DEBUG(dbgs() << "==== ==== ==== ==== ==== ==== ==== ====\n");
  //for (auto& P : Ranges)
  //  OS << *P.first<< " = " << P.second << "\n";

  //RG_DEBUG(dbgs() << "==== ==== ==== ==== SAFETY ==== ==== ==== ====\n");
  //for (auto& P : Ranges) {
  //  OS << *P.first<< " = " << P.second << "\n";
  //  bool Lower = P.second.getLower().isNotPositive();
  //  bool Upper = P.second.getUpper().isStrictlyPositive();
  //  if (Lower)
  //    OS << "Lower bound (" << P.second.getLower() << ") is non-negative.\n";
  //  if (Upper)
  //    OS << "Upper bound (" << P.second.getUpper() << ") is positive.\n";
  //  if (Lower && Upper) {
  //    if (Instruction *I = dyn_cast<Instruction>(P.first->getValue()))
  //      I->setMetadata("safe", MDNode::get(*Context,  ArrayRef<Value*>()));
  //    OS << "Safe to be loaded from or stored to\n";
  //  }
  //}

  //printAsDot(OS);
  //return false;
//}

