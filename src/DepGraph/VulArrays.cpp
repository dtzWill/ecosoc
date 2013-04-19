#define DEBUG_TYPE "VulArrays"
#include "VulArrays.h"
STATISTIC(NumVulArrays, "The number of vulnerability-causing arrays");

VulArrays::VulArrays() :
	ModulePass(ID) {
}

/** This method expects to receive a node that is input-dependent and looks for paths
 * from it to arrays. */
void VulArrays::searchForArray(Value* V) {
	std::set<GraphNode*> visitedNodes;
	std::set<Value*> alias;
	GraphNode* N = depGraph->findNode(V);
	if (N != NULL) {
		visitedNodes.clear();
		depGraph->dfsVisit(N, visitedNodes);
		for (std::set<GraphNode*>::iterator it = visitedNodes.begin(), vend =
				visitedNodes.end(); it != vend; ++it) {
			if (isa<MemNode> (*it)) {
				MemNode *M = dyn_cast<MemNode> (*it);
				alias = M->getAliases();
				for (std::set<Value*>::iterator ai = alias.begin(), aend =
						alias.end(); ai != aend; ++ai) {
					if (AllocaInst *AI = dyn_cast<AllocaInst>(*ai)) {
						Type* Ty = AI->getAllocatedType();
						if (Ty->isArrayTy()) {
							arrays.insert(*ai);
							input[*ai].insert(V);
						}
					}
				}
			}
		}
	} else {
		//		DEBUG( errs() << "Value not found in depGraph: ";
		//				errs() << *V << "\n");
	}
}

void VulArrays::findVulLocals(const Value* V) {
	const Instruction* inst = cast<Instruction> (V);
	const BasicBlock* BB = inst->getParent();
	for (BasicBlock::const_iterator I = BB->begin(), E = inst; I != E; ++I) { // While the array itself is not reached
		if (const AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
			vulLocals[V].insert(AI);
		}
	}
}

bool VulArrays::runOnModule(Module &M) {
	InputValues &IV = getAnalysis<InputValues> ();
	moduleDepGraph &m = getAnalysis<moduleDepGraph> ();
	depGraph = m.depGraph;
	std::set<Value*> depArrays;
	std::set<Value*> inputDepValues = IV.getInputDepValues();
	for (std::set<Value*>::iterator i = inputDepValues.begin(), e =
			inputDepValues.end(); i != e; ++i) {
		searchForArray(*i);
	}
	for (std::set<const Value*>::iterator i = arrays.begin(), e = arrays.end(); i
			!= e; ++i) {
		findVulLocals(*i);
	}

	for (std::set<const Value*>::iterator i = arrays.begin(), e = arrays.end(); i
			!= e;) {
		if (vulLocals[*i].size() == 0) {
			input.erase(*i);
			arrays.erase(i++);
		}
		else ++i;
	}
	NumVulArrays = arrays.size();
	DEBUG(printArrays(););
	return false;
}

void VulArrays::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<moduleDepGraph> ();
	AU.addRequired<InputValues> ();
}

void VulArrays::printArrays() {
	errs() << "[VulArrays] Vulnerability-causing arrays:\n";
	for (std::set<const Value*>::iterator i = arrays.begin(), e = arrays.end(); i != e; ++i) {
		const Function* F = cast<Instruction>(*i)->getParent()->getParent();
		errs() << "[VulArrays] Value: " << **i << " from function " << F->getName() << "\n";
		errs() << "[VulArrays] Inputs: \n";
		for (std::set<const Value*>::iterator ii = input[*i].begin(), ee = input[*i].end(); ii != ee; ++ii) {
			errs() << "[VulArrays] " << **ii << "\n";
		}
		errs() << "[VulArrays] Locals: \n";
		for (std::set<const Value*>::iterator ii = vulLocals[*i].begin(), ee = vulLocals[*i].end(); ii != ee; ++ii) {
			errs() << "[VulArrays] " << **ii << "\n";
		}
		errs() << "\n";
	}
}

std::set<const Value*> VulArrays::getVulArrays() {
	return arrays;
}

char VulArrays::ID = 0;
static RegisterPass<VulArrays> X("vul-arrays",
		"Vulnerability pass: identify arrays that cause vulnerabilities");
