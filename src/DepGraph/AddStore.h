#ifndef __ADDSTORE_H__
#define __ADDSTORE_H__

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/Instruction.h"
#include "llvm/User.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/DebugInfo.h"
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
