#ifndef DEBUG_TYPE
#define DEBUG_TYPE "hammock"
#endif


#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/SetOperations.h"
#include "llvm/Analysis/DominatorInternals.h"
#include "llvm/Assembly/Writer.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/Debug.h"
#include <deque>
#include <algorithm>
#include <vector>
#include <string>
#include <sstream>
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Operator.h"
#include "llvm/Constant.h"
#include "llvm/Constants.h"


namespace llvm {

	STATISTIC(numHammock, "Number of hammock subgraphs");
	STATISTIC(numNonHammock, "Number of NON hammock subgraphs");


	class hammock : public FunctionPass {

	public:
        	bool runOnFunction(Function&);
        	static char ID;
        	hammock() : FunctionPass(ID) {}


	private:
        	void getAnalysisUsage(AnalysisUsage &AU) const;
        	std::set<BasicBlock*> bBlocks;	//set which contains basic blocks in the influence region of an basic block
        	void processNode(BasicBlock *BB,  PostDominatorTree &PD); //start findIR
        	void findIR(BasicBlock *bBOring, BasicBlock *bBSuss, PostDominatorTree &PD); //create a subgraph in bBlocks set from an initial basic block
        	bool checkHammock (Function *f); //return true if the subgraph denoted by bBlocks set, is a hammock graph

 	};


}

