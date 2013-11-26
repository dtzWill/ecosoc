#ifndef __VUL_ARRAYS_H__
#define __VUL_ARRAYS_H__

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/User.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Metadata.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/Function.h"
#include "llvm/DebugInfo.h"
#include "llvm/ADT/StringRef.h"
#include "DepGraph.h"
#include "InputValues.h"
#include "InputDep.h"
#include "AddStore.h"
#include "bSSA.h"
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
