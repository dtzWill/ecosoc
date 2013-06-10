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
#include "DepGraph.h"
#include "llvm/ADT/StringRef.h"
#include "DepGraph.h"
#include "InputValues.h"
#include "InputDep.h"
#include "AddStore.h"
#include<set>
#include<utility>

using namespace llvm;


class VulArrays : public ModulePass {
	private:
		bool runOnModule(Module &M);
		void searchForArray(Value* V);
		void findVulLocals(const Value* V);
		bool structHasArray(StructType* ST);
		//vulnerable node n: (input value v: path from v to n)
		DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > Arrays;
		DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > Structs1;
		DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > Structs2;
		DenseMap<std::pair<GraphNode*, GraphNode*>, std::pair<unsigned, std::string> > debugInfo;
		Graph* depGraph;

		const Value* isValueInpDep(Value* V, std::set<Value*> inputDepValues);
		DenseMap<const Value*, std::vector<GraphNode*> > getValueDeps(Value* V, std::set<Value*> inputDepValues);
		void toDot(std::string name);
//		void printStats();
	public:
		static char ID;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		VulArrays();
		void printArrays();

};

#endif
