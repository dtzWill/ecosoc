#include "TFA.h"
#define DEBUG_TYPE "TFA"
#include "llvm/Support/CommandLine.h"

STATISTIC(NumTaintedNodes, "The number of nodes marked as tainted in the dep graph");
STATISTIC(NumNodes, "The number of nodes in the dep graph");

static cl::opt<llvm::cl::boolOrDefault>
		Input(
				"full",
				cl::desc(
						"Specify if every value from libraries should be treated as tainted. Default is true."),
				cl::value_desc("Input description"));

TFA::TFA() :
	ModulePass(ID) {

}
bool TFA::runOnModule(Module &M) {
	if (Input.getValue() == llvm::cl::BOU_UNSET || Input.getValue()
			== llvm::cl::BOU_TRUE) {
		InputValues &IV = getAnalysis<InputValues> ();
		inputDepValues = IV.getInputDepValues();
	} else {
		InputDep &IV = getAnalysis<InputDep> ();
		inputDepValues = IV.getInputDepValues();
	}
	bSSA &bssa = getAnalysis<bSSA> ();
	depGraph = bssa.newGraph;
	DEBUG( // display dependence graph
	string Error;
	std::string tmp = M.getModuleIdentifier();
	replace(tmp.begin(), tmp.end(), '\\', '_');
	std::string Filename = "/tmp/" + tmp + ".dot";

	//Print dependency graph (in dot format)
	depGraph->toDot(M.getModuleIdentifier(), Filename);
	DisplayGraph(Filename, true, GraphProgram::DOT);
	);
	tainted = depGraph->getDepValues(inputDepValues);
	NumTaintedNodes = tainted.size();
	NumNodes = depGraph->getNodes().size();

	DEBUG( // If debug mode is enabled, add metadata to easily identify tainted values in the llvm IR
	for (Module::iterator F = M.begin(), endF = M.end(); F != endF; ++F) {
		for (Function::iterator BB = F->begin(), endBB = F->end(); BB != endBB; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), endI = BB->end(); I
					!= endI; ++I) {
				GraphNode* g = depGraph->findNode(I);
				if (tainted.count(g)) {
					LLVMContext& C = I->getContext();
					MDNode* N = MDNode::get(C, MDString::get(C, "TFA"));
					I->setMetadata("tainted", N);
				}
			}
		}
	}
	);
	return false;
}

bool TFA::isValueTainted(Value* v) {
	GraphNode* g = depGraph->findNode(v);
	return tainted.count(g);
}

std::set<GraphNode*> TFA::getTaintedValues() {
	return tainted;
}

void TFA::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<bSSA> ();
	AU.addRequired<InputValues> ();
	AU.addRequired<InputDep> ();

}

char TFA::ID = 0;
static RegisterPass<TFA> X("tfa", "Tainted Flow Analysis");
