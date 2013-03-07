#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/ADT/ilist.h"
#include "llvm/BasicBlock.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Support/InstIterator.h"
//#include "llvm/ADT/SmallPtrSet.h"
//#include "llvm/ADT/SmallSet.h"

#include<set>

using namespace llvm;

namespace {
	struct AliasPassHelper : public ModulePass {
		static char ID;
		std::set< std::set<Value*> > finalSets;
		AliasPassHelper() : ModulePass(ID) {}
		virtual bool runOnModule(Module &M) {
			AliasSetTracker *tracker = new AliasSetTracker(getAnalysis<AliasAnalysis>());
			for (Module::iterator F = M.begin(), e = M.end(); F != e; ++F) {
				for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
					for (BasicBlock::iterator I = BB->begin(), ee = BB->end(); I != ee; I++) {
						tracker->add(&*I);
					}
				}
			}
//			int count = 0;
			std::set<Value*> aux;
			for (AliasSetTracker::const_iterator I = tracker->begin(), E = tracker->end(); I != E; ++I) {
				aux.clear();
//				++count;
//				errs() << "Set " << count << "\n";
				for (AliasSet::iterator II = I->begin(), EE = I->end(); II != EE; ++II) {
//					errs() << II->getValue() << "\n";
					aux.insert(II->getValue());
				}
				finalSets.insert(aux);
			}
			delete tracker;
			return false;
		}
		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.addRequired<AliasAnalysis>(); // transitive(?)
			AU.setPreservesAll();
		}
		std::set< std::set<Value*> > getAliasSets() {
			return finalSets;
		}
	};
}

char AliasPassHelper::ID = 0;
static RegisterPass<AliasPassHelper> X("alias-sets", "Get disjoint alias sets", false, false);
