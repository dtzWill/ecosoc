#define DEBUG_TYPE "addStore"
#include "AddStore.h"

/** Functions analyzed:
 * memcpy OK
 * memmove OK
 * memset OK
 * strcat OK
 * strncat OK
 * strcpy OK
 * strncpy OK
 * sprintf
 */

AddStore::AddStore() :
	ModulePass(ID) {
}

bool AddStore::runOnModule(Module &M) {
	moduleDepGraph &mdg = getAnalysis<moduleDepGraph> ();
	depGraph = mdg.depGraph;
	for (Module::iterator F = M.begin(), eM = M.end(); F != eM; ++F) {
		for (Function::iterator BB = F->begin(), e = F->end(); BB != e; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), ee = BB->end(); I != ee; ++I) {
				if (CallInst *CI = dyn_cast<CallInst>(I)) {
					Function *Callee = CI->getCalledFunction();
					if (Callee) {
						StringRef Name = Callee->getName();
						if (Name.equals("memcpy") || Name.equals("strcpy")
								|| Name.equals("memmove") || Name.equals(
								"memset") || Name.equals("strcat")
								|| Name.equals("strncat") || Name.equals(
								"strncpy")) { //src: arg1, dest: arg0
							Value* src = CI->getArgOperand(1);
							Value* dest = CI->getArgOperand(0);
							if (dest->getType()->isPointerTy()) {
								GraphNode* srcN = depGraph->findNode(src);
								OpNode* store = new OpNode(Instruction::Store);
								GraphNode* destN = depGraph->findNode(dest);
								depGraph->addEdge(srcN, store);
								depGraph->addEdge(store, destN);
							}
						} else if (Name.equals("sprintf")) {
							Value* dest = CI->getArgOperand(0);
							if (dest->getType()->isPointerTy()) {
								Value* src;
								GraphNode* destN = depGraph->findNode(dest);
								for (unsigned i = 2, eee =
										CI->getNumArgOperands(); i != eee; ++i) { //skip format specifier
									src = CI->getArgOperand(i);
									GraphNode* srcN = depGraph->findNode(src);
									OpNode* store = new OpNode(Instruction::Store);
									depGraph->addEdge(srcN, store);
									depGraph->addEdge(store, destN);
								}
							}
						}
					}
				}
			}
		}
	}
	return false;
}

Graph* AddStore::getModifiedGraph() {
	return depGraph;
}

void AddStore::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<moduleDepGraph> ();
	AU.setPreservesAll();
}

char AddStore::ID = 0;
static RegisterPass<AddStore> X("addStore",
		"Add Store Pass: looks for function with semantics of store operation");
