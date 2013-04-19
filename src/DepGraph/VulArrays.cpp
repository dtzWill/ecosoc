#define DEBUG_TYPE "VulArrays"
#include "VulArrays.h"
STATISTIC(NumVulArrays, "The number of vulnerability-causing arrays");

VulArrays::VulArrays() :
	ModulePass(ID) {
}

/** This method expects to receive a node that is input-dependent and looks for paths
 * from it to arrays. Side-effect on inputDepArrays */
bool VulArrays::searchForArray(Value* V) {
	std::set<GraphNode*> visitedNodes;
	std::set<Value*> alias;
	GraphNode* N = depGraph->findNode(V);
	bool flag = false;
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
							pair<const std::set<Value*>::iterator, bool> check = inputDepArrays.insert(*ai);
							flag = true;
							if (check.second) ++NumVulArrays;
						}
					}
				}
			}
		}
	} else {
		//		DEBUG( errs() << "Value not found in depGraph: ";
		//				errs() << *V << "\n";);
	}
	return flag;
}

bool VulArrays::runOnModule(Module &M) {
	InputValues &IV = getAnalysis<InputValues> ();
	moduleDepGraph &m = getAnalysis<moduleDepGraph> ();
	depGraph = m.depGraph;
	std::set<Value*> inputDepValues = IV.getInputDepValues();
	for (std::set<Value*>::iterator i = inputDepValues.begin(), e =
			inputDepValues.end(); i != e; ++i) {
//				DEBUG(errs() << "inputDepValue: "<< **i << "\n";);
		if (searchForArray(*i)) {
			inputValues.insert(*i);
		}
	}
	int count = 0;
	bool first = true;
	for (std::set<Value*>::iterator i = inputDepArrays.begin(), e =
			inputDepArrays.end(); i != e; ++i) {
//		errs() << "input dep array: " << **i << "\n";
		Instruction* inst = cast<Instruction> (*i);
		const BasicBlock* BB = inst->getParent();
		for (BasicBlock::const_iterator I = BB->begin(), E = inst; I != E; ++I) {
			if (const AllocaInst *AI = dyn_cast<AllocaInst>(I)) {
				if (first) { // Is it first local value allocated after array that is found?
					++count;
					++NumVulArrays;
					vulArrays[count] = *i;
					first = false;
				}
				vulLocals[count].insert(AI);
			}
		}
		first = true;
	}
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
	for (std::map<int, const Value*>::iterator i = vulArrays.begin(), e =
			vulArrays.end(); i != e; ++i) {
		errs() << "[VulArrays] ";
		errs() << *(i->second) << "\n";
		for (std::set<const Value*>::iterator ii = vulLocals[i->first].begin(), ee = vulLocals[i->first].end(); ii != ee; ++ii) {
			errs() << "[VulArrays]\t\t" <<**ii << "\n";
		}
	}
	errs() << "[VulArrays] Input values:\n";
	for (std::set<const Value*>::iterator i = inputValues.begin(), e = inputValues.end(); i != e; ++i) {
		 errs() << "[VulArrays] " << **i << "\n" ;
	}
	errs() << "\n[VulArrays] *END*\n";
}

std::map<int, const Value*> VulArrays::getVulArrays() {
	return vulArrays;
}

char VulArrays::ID = 0;
static RegisterPass<VulArrays> X("vul-arrays",
		"Vulnerability pass: identify arrays that cause vulnerabilities");
