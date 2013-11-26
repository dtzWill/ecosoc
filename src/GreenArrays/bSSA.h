#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/PostOrderIterator.h"
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
#include "DepGraph.h"
#include <sstream>

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "bSSA"
#endif

using namespace std;

namespace llvm {

	class Pred {
		Value *predicate;
		std::vector<Instruction *> insts;
		std::vector<Function *> funcs;

	public:
		Pred(Value *p);	//Constructor
		void addInst (Instruction *i);
		void addFunc (Function *f);
		int getNumInstrucoes ();
		int getNumFunctions ();
		Value *getPred ();
		Instruction *getInst (int i);
		Function *getFunc (int i);
		bool isGated (Instruction *);
	};



	class bSSA : public ModulePass {
			//src stores the instructions which are source of secret information
			//dst stores the instructions  which are public output channel like printf()
			std::vector<Value *> src, dst;
	public:

        	static char ID;
        	bSSA() : ModulePass(ID) {}
        	void getAnalysisUsage(AnalysisUsage &AU) const;
        	bool runOnModule(Module&);
        	void printGate ();	//Print the predicates and its respective gated instructions
        	void incGraph (Graph *g); //Increase graph including control edges
        	Graph *newGraph;
        	std::set<Value *> getLeakedValues (); //Converte os subgrafos com nós sujos marcados para um conjunto de nós.
        	std::vector<std::pair<Instruction*, std::vector<Value*> > > getPrintfLeaks ();

	private:
        	std::vector<Pred *> predicatesVector;	//Vector of predicates objects
        	void makeTable (BasicBlock *b, PostDominatorTree& PD);
        	void findIR (BasicBlock *borigin, BasicBlock *bdst, PostDominatorTree &PD);	//Run on Influence Region of basic block borigin gated all instructions
        	void gateFunction (Function *F, Pred *p);
 	};


}

