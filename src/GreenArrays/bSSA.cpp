#define DEBUG_TYPE "bssa"

#include "bSSA.h"

using namespace llvm;

std::set<BasicBlock *> GatedBB;
std::vector<BasicBlock *> ProcessedBB;

//Receive a predicate and include it on predicate attribute
Pred::Pred(Value *p) {
	predicate = p;
}

//Receive a instruction and include it on insts vector
void Pred::addInst(Instruction *i) {
	insts.push_back(i);
}

//Receive a function pointer and include it on funcs vector
void Pred::addFunc(Function *f) {
	funcs.push_back(f);
}

//Return the total instruction count
int Pred::getNumInstrucoes() {
	return (insts.size());
}

int Pred::getNumFunctions() {
	return (funcs.size());
}

//Return the predicate
Value *Pred::getPred() {
	return (predicate);
}

//Return the instruction stored on insts vector, pointed for parameter i
Instruction *Pred::getInst(int i) {
	if (i < (signed int) insts.size())
		return (insts[i]);
	else
		return NULL;
}

Function *Pred::getFunc(int i) {
	if (i < (signed int) funcs.size())
		return (funcs[i]);
	else
		return NULL;
}

//Return true of *op instruction is gated (if it is stored on insts vector) for the predicate
bool Pred::isGated(Instruction *op) {
	unsigned int i;

	for (i = 0; i < insts.size(); i++) {
		if (op == insts[i])
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//Passes which are used by bSSA pass
void bSSA::getAnalysisUsage(AnalysisUsage &AU) const {

	AU.addRequired<PostDominatorTree> ();
	AU.addRequired<DominatorTree>();
	AU.addRequired<moduleDepGraph> ();

	// This pass will not modifies the program nor CFG
	AU.setPreservesAll();

}


//Retorna um vetor de pares ordenados (Instrução de print, vetor de parâmetros)
std::vector<std::pair<Instruction*, std::vector<Value*> > > bSSA::getPrintfLeaks() {

	std::vector<std::pair<Instruction*, std::vector<Value*> > > v;

	std::vector<Value *> args;

	for (std::vector<Value *>::iterator dstIt = dst.begin(), dstE = dst.end(); dstIt
			!= dstE; ++dstIt) {
		if (Instruction *I = dyn_cast<Instruction>(*dstIt)) {
			if (CallInst *cI = dyn_cast<CallInst>(*dstIt)) {
				for (unsigned i = 0; i < cI->getNumArgOperands(); i++) {
					args.push_back(cI->getArgOperand(i));
				}
				v.push_back(
						std::pair<Instruction*, std::vector<Value*> >(I, args));
			}
		}
	}
	return (v);
}

//For each module, this method is executed
bool bSSA::runOnModule(Module &M) {
	moduleDepGraph& DepGraph = getAnalysis<moduleDepGraph> ();
	Function *F;
	//Getting dependency graph
	Graph *g = DepGraph.depGraph;

	for (Module::iterator Mit = M.begin(), Mend = M.end(); Mit != Mend; ++Mit) {
		F = Mit;
		// Iterate over all Basic Blocks of the Function
		if (F->begin() != F->end()) {
			DominatorTree &DT = getAnalysis<DominatorTree> (*F);
			PostDominatorTree &PD = getAnalysis<PostDominatorTree> (*F);
			GatedBB.clear();
			for (po_iterator<DomTreeNode*> Fit = po_begin(DT.getRootNode()),
					Fend = po_end(DT.getRootNode()); Fit != Fend; Fit++) {
				makeTable(Fit->getBlock(), PD); //Creating in memory the table with predicates and gated instructions
			}
		}
	}

	//Including control edges into dependence graph.
	incGraph(g);
	newGraph = g;
	return false;
}

//Increase graph including control edges
void bSSA::incGraph(Graph *g) {
	unsigned int i;
	int j;

	//For all predicates in predicatesVector
	for (i = 0; i < predicatesVector.size(); i++) {
		//Locates the predicate (icmp instrution) Localiza o predicado (instrução icmp) from the graph
		GraphNode *predNode = g->findNode(predicatesVector[i]->getPred());

		//For each predicate, iterates on the list of gated INSTRUCTIONS
		for (j = 0; j < predicatesVector[i]->getNumInstrucoes(); j++) {
			GraphNode *instNode = g->findNode(predicatesVector[i]->getInst(j));
			if (predNode != NULL && instNode != NULL) {//If the instruction is on the graph, make a edge
				g->addEdge(predNode, instNode, etControl);
			}
		}

		//For each predicate, iterates on the list of gated FUNCTIONS
		for (j = 0; j < predicatesVector[i]->getNumFunctions(); j++) {
			Function *F = predicatesVector[i]->getFunc(j);
			//For each function, iterates on its basic blocks
			for (Function::iterator Fit = F->begin(), Fend = F->end(); Fit
					!= Fend; ++Fit) {
				//For each basic block, iterates on its instructions
				for (BasicBlock::iterator bBIt = Fit->begin(), bBEnd =
						Fit->end(); bBIt != bBEnd; ++bBIt) {
					GraphNode *instNode = g->findNode(bBIt);
					if (predNode != NULL && instNode != NULL)
						g->addEdge(predNode, instNode, etControl);
				}
			}
		}

	}

}

//It receives a BasicBLock and makes table of predicates and its respective gated instructions
void bSSA::makeTable(BasicBlock *BB, PostDominatorTree &PD) {
	Value *condition;
	TerminatorInst *ti = BB->getTerminator();
	BranchInst *bi = NULL;
	SwitchInst *si = NULL;

	//        PostDominatorTree &PD = getAnalysis<PostDominatorTree>(*F);

	ProcessedBB.clear();
	if ((bi = dyn_cast<BranchInst> (ti)) && bi->isConditional()) { //If the terminator instruction is a conditional branch
		condition = bi->getCondition();
		//Including the predicate on the predicatesVector
		predicatesVector.push_back(new Pred(condition));
		//Make a "Flooding" on each sucessor gated the instruction on Influence Region of the predicate
		for (unsigned int i = 0; i < bi->getNumSuccessors(); i++) {
			findIR(BB, bi->getSuccessor(i), PD);
		}
	} else if ((si = dyn_cast<SwitchInst> (ti))) {
		condition = si->getCondition();
		//Including the predicate on the predicatesVector
		predicatesVector.push_back(new Pred(condition));
		//Make a "Flooding" on each sucessor gated the instruction on Influence Region of the predicate
		for (unsigned int i = 0; i < si->getNumSuccessors(); i++) {
			findIR(BB, si->getSuccessor(i), PD);
		}
	}
}

//Flooding until reach a posdominator node
void bSSA::findIR(BasicBlock *bBOring, BasicBlock *bBSuss,
		PostDominatorTree &PD) {

	Pred *p;
	TerminatorInst *ti = bBSuss->getTerminator();

	//If the basic block has been processed, do not advance
	for (unsigned int x = 0; x < ProcessedBB.size(); x++) {
		if (ProcessedBB[x] == bBSuss) {
			return;
		}
	}

	//Including the basic block in the processed vector
	ProcessedBB.push_back(bBSuss);

	//If the basic block is a posdominator and is not the start basic block, just gate the PHI instructions
	if (PD.dominates(bBSuss, bBOring) && bBSuss != bBOring) {
		//Find PHI instructions
		for (BasicBlock::iterator bBIt = bBSuss->begin(), bBEnd = bBSuss->end(); bBIt
				!= bBEnd; ++bBIt) {
			if (dyn_cast<Instruction> (bBIt)->getOpcode() == Instruction::PHI) {
				//gate the PHI instruction
				p = predicatesVector.back();
				p->addInst(bBIt);
			}
		}
		return;
	} else { //Advance the flooding
		//Instruction will be gated whit the bBOring predicate
		for (BasicBlock::iterator bBIt = bBSuss->begin(), bBEnd = bBSuss->end(); bBIt
				!= bBEnd; ++bBIt) {
			p = predicatesVector.back();

			//If is a function call which is defined on the same module
			if (CallInst *CI = dyn_cast<CallInst>(&(*bBIt))) {
				Function *F = CI->getCalledFunction();
				if (F != NULL)
					if (!F->isDeclaration() && !F->isIntrinsic()) {
						gateFunction(F, p);
					}
			}

			//Gate the other instructions
			p->addInst(bBIt);
		}

		//If is a basic block which its IR has been processed and it has a conditional instruction, do not advance
		if (GatedBB.count(bBSuss) > 0) {
			TerminatorInst *ti = bBSuss->getTerminator();
			BranchInst *bi;
			if ((bi = dyn_cast<BranchInst> (ti)) && bi->isConditional())
				return;
		}
		GatedBB.insert(bBSuss);

		//If there is successor, go there
		for (unsigned int i = 0; i < ti->getNumSuccessors(); i++) {
			findIR(bBOring, ti->getSuccessor(i), PD);
		}
	}

}

//All instrutions of function F are gated with predicate p
void bSSA::gateFunction(Function *F, Pred *p) {
	// Iterate over all Basic Blocks of the Function
	//for (Function::iterator Fit = F->begin(), Fend = F->end(); Fit != Fend; ++Fit) {
	//for (BasicBlock::iterator bBIt = Fit->begin(), bBEnd = Fit->end(); bBIt != bBEnd; ++bBIt) {
	p->addFunc(F);
	//}
	//}
}

//Print all predicates and its respective gated instructions
void bSSA::printGate() {
	for (unsigned int i = 0; i < predicatesVector.size(); i++) {
		errs() << "\n\n" << predicatesVector[i]->getPred()->getName() << "\n";
		for (int j = 0; j < predicatesVector[i]->getNumInstrucoes(); j++) {
			errs() << predicatesVector[i]->getInst(j)->getOpcodeName() << "\n";

		}
	}

}

char bSSA::ID = 0;
static RegisterPass<bSSA> X("bssa", "B Assignment Construction");

