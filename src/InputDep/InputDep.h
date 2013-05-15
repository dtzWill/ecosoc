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


class InputDep : public ModulePass {
	private:
		std::set<Value*> inputDepValues;
		bool runOnModule(Module &M);
	public:
		static char ID; // Pass identification, replacement for typeid.
		InputDep();
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool isInputDependent(Value* V);
		std::set<Value*> getInputDepValues();
};

#endif
