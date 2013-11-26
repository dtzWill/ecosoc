//===------------------------ TFA.cpp --------------------------===//
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

#include "TFA.h"

#include <chrono>
#include <iomanip>
#include <queue>
#include <set>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

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

/*******************
 * AnnotateTainted *
 *******************/
class AnnotateTainted : public ModulePass {
public:
  static char ID;
  AnnotateTainted() : ModulePass(ID) { }

  virtual void getAnalysisUsage(AnalysisUsage& AU) const;
  virtual bool runOnModule(Module& M);

private:
  void setMetadataOn(Instruction *I); 

  void visitStore(StoreInst *SI);
  void visitLoad(LoadInst *LI);

  LLVMContext *Context_;
  TFA *TFA_;

  unsigned SafeStores_;
  unsigned TotalStores_;
  unsigned SafeLoads_;
  unsigned TotalLoads_;
};

void AnnotateTainted::getAnalysisUsage(AnalysisUsage& AU) const {
  AU.addRequired<TFA>();
  AU.setPreservesAll();
}

bool AnnotateTainted::runOnModule(Module& M) {
  Context_ = &M.getContext();

  TFA_ = &getAnalysis<TFA>();

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

  dbgs() << "================ TAINT STATS ===============\n";
  dbgs() << "==== Number of loads:          "  << TotalLoads_          << "\n";
  dbgs() << "==== Number of safe loads:     "  << SafeLoads_           << "\n";
  dbgs() << "==== \% of safe loads:          " << PercentageSafeLoads  << "\n";
  dbgs() << "==== Number of stores:         "  << TotalStores_         << "\n";
  dbgs() << "==== Number of safe stores:    "  << SafeStores_          << "\n";
  dbgs() << "==== \% of safe stores:         " << PercentageSafeStores << "\n";
  dbgs() << "==== \% of safe accesses:       " << PercentageTotalSafe  << "\n";

  return false;
}

void AnnotateTainted::setMetadataOn(Instruction *I) { 
  I->setMetadata("memsafe", MDNode::get(*Context_,  ArrayRef<Value*>()));
}

void AnnotateTainted::visitStore(StoreInst *SI) {
  TotalStores_++;
  if (SI->getMetadata("memsafe"))
    return;
  Value *Ptr = SI->getPointerOperand();
  if (!TFA_->isValueTainted(Ptr)) {
    SafeStores_++;
    setMetadataOn(SI);
  }
}

void AnnotateTainted::visitLoad(LoadInst *LI) {
  TotalLoads_++;
  if (LI->getMetadata("memsafe"))
    return;
  Value *Ptr = LI->getPointerOperand();
  if (!TFA_->isValueTainted(Ptr)) {
    SafeStores_++;
    setMetadataOn(LI);
  }
}

char AnnotateTainted::ID = 0;
static RegisterPass<AnnotateTainted>
  R("tainted-annotate", "Annotate safe-to-dereference values",
    true, true);

