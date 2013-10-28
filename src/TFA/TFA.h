#ifndef __VUL_ARRAYS_H__
#define __VUL_ARRAYS_H__

#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/BasicBlock.h"
#include "llvm/Instruction.h"
#include "llvm/User.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Instructions.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Metadata.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Function.h"
#include "llvm/DebugInfo.h"
#include "llvm/ADT/StringRef.h"
#include "../DepGraph/DepGraph.h"
#include "../DepGraph/InputValues.h"
#include "../DepGraph/InputDep.h"
#include "../DepGraph/AddStore.h"
#include "../bSSA/bSSA.h"
#include<set>
#include<utility>

using namespace llvm;


class TFA : public ModulePass {
	private:
		Graph* depGraph;
		std::set<Value*> inputDepValues;
		bool runOnModule(Module &M);
		bool isValueInpDep(Value* V);
		std::set<GraphNode*> tainted;
	public:
		static char ID;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		std::set<GraphNode*> getTaintedValues();
		bool isValueTainted(Value* v);
		TFA();

};

#endif
