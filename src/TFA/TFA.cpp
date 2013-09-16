#include "TFA.h"
#define DEBUG_TYPE "TFA"
#include "llvm/Support/CommandLine.h"

static cl::opt<bool>
		Input(
				"full",
				cl::desc(
						"Specify if every value from libraries should be treated as tainted. Default is false."),
				cl::value_desc("Input description"));

TFA::TFA() :
	ModulePass(ID) {

}
bool TFA::runOnModule(Module &M) {
	if (Input.getValue() == true) {
		InputValues &IV = getAnalysis<InputValues> ();
		inputDepValues = IV.getInputDepValues();
	} else {
		InputDep &IV = getAnalysis<InputDep> ();
		inputDepValues = IV.getInputDepValues();
//		IV.printer();
	}
	AddStore &AS = getAnalysis<AddStore> ();
	depGraph = AS.getModifiedGraph();
	std::set<Value*> tainted = depGraph->getDepValues(inputDepValues);
	for (Module::iterator F = M.begin(), endF = M.end(); F != endF; ++F) {
		for (Function::iterator BB = F->begin(), endBB = F->end(); BB != endBB; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), endI = BB->end(); I
					!= endI; ++I) {
				if (tainted.count(I)) {
					LLVMContext& C = I->getContext();
					MDNode* N = MDNode::get(C, MDString::get(C, "TFA"));
					I->setMetadata("tainted", N);
				}
			}
		}
	}
	return false;
}

void TFA::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<AddStore> ();
	AU.addRequired<InputValues> ();
	AU.addRequired<InputDep> ();
}

char TFA::ID = 0;
static RegisterPass<TFA> X("tfa", "Tainted Flow Analysis");
