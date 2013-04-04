#ifndef __INPUTDEP_H__
#define __INPUTDEP_H__

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/Instruction.h"
#include "llvm/User.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ilist.h"
#include "llvm/Instructions.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Metadata.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Function.h"
#include "llvm/DepGraph.h"
#include "llvm/ADT/StringRef.h"
#include<set>

using namespace llvm;
//};

class InputDep : public ModulePass {
	private:
		Graph* depGraph;
		bool runOnModule(Module &M);
		void searchForArray(Value* V);
		bool verifySignature(CallInst &CI, const StringRef &name);
		std::set<Value*> inputDepArrays;
	public:
		static char ID;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		InputDep();
		void printArrays();
		std::set<Value*> getInputDepArrays();

};

#endif
