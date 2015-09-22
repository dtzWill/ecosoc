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
	netDepGraph &mdg = getAnalysis<netDepGraph> ();
	depGraph = mdg.depGraph;
	for (Module::iterator F = M.begin(), eM = M.end(); F != eM; ++F) {
        Function *f = F;
        StringRef fname = f->getName();
        int programID=1;
        if(fname.startswith("ttt_"))//Program #2
            programID=2;
        else programID=1;
		for (Function::iterator BB = F->begin(), e = F->end(); BB != e; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), ee = BB->end(); I != ee; ++I) {
				if (CallInst *CI = dyn_cast<CallInst>(I)) {
					Function *Callee = CI->getCalledFunction();                    
					if (Callee) {
						StringRef Name = Callee->getName();
						if (	Name.equals("memcpy") || Name.equals("ttt_memcpy") || 
							Name.equals("strcpy") || Name.equals("ttt_strcpy") || 
							Name.equals("memmove") || Name.equals("ttt_memmove") || 
							Name.equals("memset") || Name.equals("ttt_memset") || 
							Name.equals("strcat") || Name.equals("ttt_strcat") || 
							Name.equals("strncat") || Name.equals("ttt_strncat") || 
							Name.equals( "strncpy") || Name.equals( "strncpy")) { //src: arg1, dest: arg0
							Value* src = CI->getArgOperand(1);
							Value* dest = CI->getArgOperand(0);
							if (dest->getType()->isPointerTy()) {

                                GraphNode* srcN = depGraph->findNode(src,programID);
								OpNode* store = new OpNode(Instruction::Store);
                                GraphNode* destN;
                                destN = depGraph->findNode(dest,programID);
								depGraph->addEdge(programID, srcN, store);
                                depGraph->addEdge(programID, store, destN);
							}
						} else if (Name.equals("sprintf")||Name.equals("ttt_sprintf")) {
							Value* dest = CI->getArgOperand(0);
							if (dest->getType()->isPointerTy()) {
								Value* src;
                                GraphNode* destN = depGraph->findNode(dest,programID);
								for (unsigned i = 2, eee =
										CI->getNumArgOperands(); i != eee; ++i) { //skip format specifier
									src = CI->getArgOperand(i);
                                    GraphNode* srcN = depGraph->findNode(src,programID);
									OpNode* store = new OpNode(Instruction::Store);
									depGraph->addEdge(programID, srcN, store);
									depGraph->addEdge(programID, store, destN);
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
	AU.addRequired<netDepGraph> ();
//	AU.setPreservesAll();
	AU.addPreservedID(InputDep::ID);
//	AU.addPreservedID(AliasSets::ID);
	AU.addPreservedID(PADriver::ID);

}

char AddStore::ID = 0;
static RegisterPass<AddStore> X("addStore",
		"Add Store Pass: looks for function with semantics of store operation");
