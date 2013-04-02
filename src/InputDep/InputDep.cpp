#define DEBUG_TYPE "vulnerable"
#include "InputDep.h"

InputDep::InputDep() : FunctionPass(ID) {}

bool InputDep::runOnFunction(Function &F) {
	DEBUG (errs() << "Function " << F.getName() << "\n";);
	moduleDepGraph &DG = getAnalysis<moduleDepGraph>();
	Graph* depGraph = DG.depGraph;
	std::set<GraphNode*> visitedNodes;
	std::set<Value*> alias;
	for (Function::iterator BB = F.begin(), e = F.end(); BB != e; ++BB) {
		for (BasicBlock::iterator I = BB->begin(), ee = BB->end(); I != ee; ++I) {
			if (CallInst *CI = dyn_cast<CallInst>(I)) {
				Function *fun = CI->getCalledFunction();
				if (fun) {
					if (fun->getName().equals(StringRef("__isoc99_scanf"))) {
						for (unsigned i = 1, eee = CI->getNumArgOperands(); i != eee; ++i) { // skip format string (i=1)
							Value* V = CI->getArgOperand(i);
							GraphNode* N = depGraph->findNode(V);
							visitedNodes.clear();
							depGraph->dfsVisit (N, visitedNodes);
							for (std::set<GraphNode*>::iterator it = visitedNodes.begin(), vend = visitedNodes.end(); it != vend; ++it) {
								if (isa<MemNode>(*it)){
									MemNode *M = dyn_cast<MemNode>(*it);
									alias = M->getAliases();
									for (std::set<Value*>::iterator ai = alias.begin(), aend = alias.end(); ai != aend; ++ai) {
//										errs() << "*\n";
										if (AllocaInst *AI = dyn_cast<AllocaInst>(*ai)) {
											Type* Ty = AI->getAllocatedType();
											if (Ty->isArrayTy()) {
												errs() << "Found! :)" << **ai << "\n";
											}
											else {
												errs() << " not found!" << **ai << "\n";
											}
										}
									}
								}
							}
						}
					}
					else {}
				}
				else
					errs() << "indirect call\n";
			}
		}
	}
	return false;
}

void InputDep::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<moduleDepGraph>();
}

char InputDep::ID = 0;
static RegisterPass<InputDep> X("input-dep", "Input Dependency Pass: looks for buffers that are input-dependant");
