#define DEBUG_TYPE "hammock"

#include "hammock.h"

using namespace llvm;


void hammock::getAnalysisUsage(AnalysisUsage &AU) const {
		AU.addRequired<PostDominatorTree>();
        // This pass will not modifies the program nor CFG
        AU.setPreservesAll();

}

//For each function, this method is executed
bool hammock::runOnFunction(Function &F) {

		functionIsHammock = true;

		PostDominatorTree &PD = getAnalysis<PostDominatorTree>();
		for (Function::iterator Fit = F.begin(), Fend = F.end(); Fit != Fend; ++Fit) {
			//Mark BasicBlock
			bBlocks.insert(Fit);
			//Find Influence Region of the BasicBlock
			processNode(Fit, PD);
			//Check if some unmarked basic block goes to influence region
			if (checkHammock(F)) {
					++numHammock;

			} else {
					++numNonHammock;
					functionIsHammock = false;
			}
			//Unmark all nodes
			bBlocks.empty();
		}

	return false;
}

//Return true if the subgraph denoted by bBlocks set attribute is a hammock graph
bool hammock::checkHammock (Function &F) {
	//For each basicBlock
	for (Function::iterator Fit = F.begin(), Fend = F.end(); Fit != Fend; ++Fit) {
		TerminatorInst *ti = Fit->getTerminator();

		if (bBlocks.count(Fit)==0) { //If it is out of subgraph
			//Check if some respective successor is marked
			for (unsigned int i=0; i<ti->getNumSuccessors(); i++) {
				if (bBlocks.count(ti->getSuccessor(i))>0) {
					return false;
				}
			}
		}else { /*//If it is in subgrah
			//Check if some respective successor is NOT marked
			for (unsigned int i=0; i<ti->getNumSuccessors(); i++) {
				if (bBlocks.count(ti->getSuccessor(i))==0) {
					return false;
				}
			}*/
		}
	}

	return true;
}

//Start findIR method
void hammock::processNode (BasicBlock *BB,  PostDominatorTree &PD ) {

  		TerminatorInst *ti = BB->getTerminator();
        BranchInst *bi = NULL;
        SwitchInst *si=NULL;


        if ((bi = dyn_cast<BranchInst>(ti)) && bi->isConditional()) { //If the terminator instruction is a conditional branch
            for (unsigned int i=0; i<bi->getNumSuccessors(); i++) {
                 findIR (BB, bi->getSuccessor(i),PD);
            }
        }else if ((si = dyn_cast<SwitchInst>(ti))) {
		    for (unsigned int i=0; i<si->getNumSuccessors(); i++) {
		        findIR (BB, si->getSuccessor(i),PD);
		    }
        }
}

void hammock::findIR (BasicBlock *bBOring, BasicBlock *bBSuss, PostDominatorTree &PD) {

	TerminatorInst *ti = bBSuss->getTerminator();


	if (bBlocks.count(bBSuss)>0) {
		return;
	}

	//Mark BasicBlock
	bBlocks.insert(bBSuss);

	//If the basic block is a posdominator and is not the start basic block, just return
	if (PD.dominates(bBSuss, bBOring) && bBSuss != bBOring) {
		return;
	}else { //Advance the flooding
		//If there is successor, go there
		for (unsigned int i=0; i<ti->getNumSuccessors(); i++) {
			findIR (bBOring, ti->getSuccessor(i), PD);
		}
	}

}

char hammock::ID = 0;
static RegisterPass<hammock> X("hammock", "hammock verify");




