#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "../DepGraph/DepGraph.h"
#include <deque>
#include <algorithm>
#include <vector>
#include <string>

using namespace std;

namespace llvm {
	class Pred {
		ICmpInst *predicado;
		std::vector<Instruction *> instrucoes;

	public:
		Pred(ICmpInst *p);
		void insereInstrucao (Instruction *i);
		int getNumInstrucoes ();
	};

	class bSSA : public FunctionPass {
	public:
        	static char ID; // Pass identification, replacement for typeid.
        	bSSA() : FunctionPass(ID) {}
        	void getAnalysisUsage(AnalysisUsage &AU) const;
        	bool runOnFunction(Function&);

	private:
		std::vector<Pred *> predicados;
        	// Variables always live
		void makeTable (BasicBlock *b);
	};


	class bSSA_module : public ModulePass {
	public:
        	static char ID; // Pass identification, replacement for typeid.
        	bSSA_module() : ModulePass(ID) {}
        	void getAnalysisUsage(AnalysisUsage &AU) const;
        	bool runOnModule(Module&);

	};


}

