#ifndef __ADDSTORE_H__
#define __ADDSTORE_H__

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/User.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/DebugInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ilist.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Metadata.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/Function.h"
#include "llvm/ADT/StringRef.h"
#include "InputDep.h"
#include "DepGraph.h"
#include "PADriver.h"
#include "AliasSets.h"
#include<set>

using namespace llvm;


class AddStore : public ModulePass {
	private:
		bool runOnModule(Module &M);
		Graph* depGraph;
	public:
		static char ID;
		AddStore();
		void getAnalysisUsage(AnalysisUsage &AU) const;
		Graph* getModifiedGraph();
};

#endif
