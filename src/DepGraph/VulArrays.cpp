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
		if (isa<MemNode> (N)) {
			MemNode *M = dyn_cast<MemNode> (N);
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
	//	Look for arrays passed as parameters
	for (std::set<Value*>::iterator i = inputDepValues.begin(), e =
			inputDepValues.end(); i != e; ++i) {
		searchForArray(*i);
	}
	//Look for arrays that are assigned to directly
	for (Module::iterator F = M.begin(), endM = M.end(); F != endM; ++F) {
		for (Function::iterator BB = F->begin(), endBB = F->end(); BB != endBB; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), endI = BB->end(); I
					!= endI; ++I) {
				if (AllocaInst* AI = dyn_cast<AllocaInst>(I)) {
					Type* Ty = AI->getAllocatedType();
					if (Ty->isArrayTy()) {
						GraphNode* N = depGraph->findNode(cast<Value> (I));
						std::map<GraphNode*, edgeType> pred =
								N->getPredecessors();
						for (std::map<GraphNode*, edgeType>::iterator i =
								pred.begin(), endi = pred.end(); i != endi; ++i) {
							GraphNode* n = i->first;
							if (OpNode* ON = dyn_cast<OpNode> (n)) {
								if (ON->getOpCode() == Instruction::Store) {
									std::pair<GraphNode*, int> dep =
											depGraph->getNearestDependency(
													ON->getValue(),
													inputDepValues, false);
									if (dep.first != NULL) {
										arrays.insert(AI);
										if (VarNode* VN = dyn_cast<VarNode>(dep.first)) {
											input[AI].insert(VN->getValue());
										}
//										else if (MemNode* MN = dyn_cast<MemNode>(dep.first)) {
											//Put all the alias set? Might be of no use at all
//										}
									}
								}
							}
						}
					}
				}
			}
		}
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
		} else
			++i;
	}
	NumVulArrays = arrays.size();
	printArrays();
	return false;
}

void VulArrays::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<moduleDepGraph> ();
	AU.addRequired<InputValues> ();
}

void VulArrays::printArrays() {
	errs() << "[VulArrays] Vulnerability-causing arrays:\n";
	for (std::set<const Value*>::iterator i = arrays.begin(), e = arrays.end(); i
			!= e; ++i) {
		const Function* F = cast<Instruction> (*i)->getParent()->getParent();
		errs() << "[VulArrays] Value: " << **i << " from function "
				<< F->getName() << "\n";
		errs() << "[VulArrays] Inputs: \n";
		for (std::set<const Value*>::iterator ii = input[*i].begin(), ee =
				input[*i].end(); ii != ee; ++ii) {
			errs() << "[VulArrays] " << **ii << "\n";
		}
		errs() << "[VulArrays] Locals: \n";
		for (std::set<const Value*>::iterator ii = vulLocals[*i].begin(), ee =
				vulLocals[*i].end(); ii != ee; ++ii) {
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
