#define DEBUG_TYPE "vulnerable"
#include "InputDep.h"

InputDep::InputDep() :
	ModulePass(ID) {
}

bool InputDep::verifySignature(CallInst &CI, const StringRef &name) {
	//	TODO: Check if function really looks like libc function to avoid misinterpret user redefinition
	//	http://llvm.org/docs/doxygen/html/MemoryBuiltins_8cpp_source.html line 123
	return true;
}

/** This method expects to receive a node that is input-dependent and looks for paths
 * from it to arrays */
void InputDep::searchForArray(Value* V) {
	std::set<GraphNode*> visitedNodes;
	std::set<Value*> alias;
	GraphNode* N = depGraph->findNode(V);
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
						inputDepArrays.insert(*ai);
						//						DEBUG(errs() << "Found **ai" << **ai << "\n";);
					}
				}
			}
		}
	}
}

bool InputDep::runOnModule(Module &M) {
	//	DEBUG (errs() << "Function " << F.getName() << "\n";);
	moduleDepGraph &DG = getAnalysis<moduleDepGraph> ();
	depGraph = DG.depGraph;
	for (Module::iterator F = M.begin(), eM = M.end(); F != eM; ++F) {
		for (Function::iterator BB = F->begin(), e = F->end(); BB != e; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), ee = BB->end(); I != ee; ++I) {
				if (CallInst *CI = dyn_cast<CallInst>(I)) {
					Function *Callee = CI->getCalledFunction();
					if (Callee) {
						if (Callee->getName().equals(
								StringRef("__isoc99_scanf"))
								&& verifySignature(*CI, Callee->getName())) {
							for (unsigned i = 1, eee = CI->getNumArgOperands(); i
									!= eee; ++i) { // skip format string (i=1)
								Value* V = CI->getArgOperand(i);
								searchForArray(V);
							}
						} else if (Callee->getName().equals(
								StringRef("@__isoc99_fscanf"))
								&& verifySignature(*CI, Callee->getName())) {
							for (unsigned i = 2, eee = CI->getNumArgOperands(); i
									!= eee; ++i) { // skip file pointer and format string (i=1)
								Value* V = CI->getArgOperand(i);
								searchForArray(V);
							}
						} else if ((Callee->getName().equals(StringRef("gets"))
								|| Callee->getName().equals(StringRef("fgets"))
								|| Callee->getName().equals(StringRef("fread")))
								&& verifySignature(*CI, Callee->getName())) {
							Value* V = CI->getArgOperand(0); //the first argument receives the input for these functions
							searchForArray(V);
						} else if ((Callee->getName().equals(StringRef("fgetc"))
								|| Callee->getName().equals(StringRef("getc"))
								|| Callee->getName().equals(
										StringRef("getchar")))
								&& verifySignature(*CI, Callee->getName())) {
							searchForArray(CI);
						}
					} else
						errs() << "indirect call\n"; // Ponteiro pra função: verificar alias?
				}
			}
		}
	}
	DEBUG(printArrays(););
	return false;
}

void InputDep::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<moduleDepGraph> ();
}

void InputDep::printArrays() {
	errs() << "Input-dependent arrays:\n";
	for (std::set<Value*>::iterator i = inputDepArrays.begin(), e =
			inputDepArrays.end(); i != e; ++i) {
		errs() << **i << "\n";
	}
}

std::set<Value*> InputDep::getInputDepArrays() {
	return inputDepArrays;
}

char InputDep::ID = 0;
static RegisterPass<InputDep> X("input-dep",
		"Input Dependency Pass: looks for buffers that are input-dependant");
