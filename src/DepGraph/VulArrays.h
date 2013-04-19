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
#include "llvm/Instructions.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Metadata.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Function.h"
#include "llvm/DepGraph.h"
#include "llvm/ADT/StringRef.h"
#include "DepGraph.h"
#include "InputValues.h"
#include<set>

using namespace llvm;


class VulArrays : public ModulePass {
	private:
		bool runOnModule(Module &M);
		bool searchForArray(Value* V);
		Graph* depGraph;
		std::set<Value*> inputDepArrays;
		std::map<int, const Value*> vulArrays;
		std::map<int, std::set<const Value*> > vulLocals;
		std::set<const Value*> inputValues; // stores input values of interest (that cause vulnerab.)
	public:
		static char ID;
//		Graph* depGraph;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		VulArrays();
		void printArrays();
		std::map<int, const Value*> getVulArrays();

};

#endif
