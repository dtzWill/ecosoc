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

bool TFA::isValueInpDep(Value* V) {
	//Firstly, check if array or alias is passed as parameter to "any" lib function
	std::set<Value*> alias;
	static DenseMap<GraphNode*, bool> isDep; // to avoid repeated computation
	GraphNode* N = depGraph->findNode(V);
	if (N == NULL)
		DEBUG(errs() << "Error: node not found in dep graph.\n");
	return true;
	if (isDep.count(N)) {
		return isDep[N];
	}
	if (isa<MemNode> (N)) {
		MemNode *M = dyn_cast<MemNode> (N);
		alias = M->getAliases();
		for (std::set<Value*>::iterator ai = alias.begin(), aend = alias.end(); ai
				!= aend; ++ai) {
			if (inputDepValues.count(*ai)) {
				isDep[N] = true;
				return true;
			}
		}
	}

	//Secondly, check store operations targeting the array
	std::map<GraphNode*, edgeType> pred = N->getPredecessors();
	for (std::map<GraphNode*, edgeType>::iterator i = pred.begin(), endi =
			pred.end(); i != endi; ++i) {
		GraphNode* n = i->first;
		if (OpNode* ON = dyn_cast<OpNode> (n)) {
			if (ON->getOpCode() == Instruction::Store) {
				std::pair<GraphNode*, int> dep =
						depGraph->getNearestDependency(ON->getValue(),
								inputDepValues, false);
				if (dep.first != NULL) {
					if (VarNode * VN = dyn_cast<VarNode> (dep.first)) {
						isDep[N] = VN->getValue();
						return VN->getValue();
					} else if (MemNode * MN = dyn_cast<MemNode> (dep.first)) {
						std::set<Value*>::iterator i = MN->getAliases().begin();
						isDep[N] = *i; //get alias 0 as representative
						return true;
					}
					DEBUG(errs() << "[TFA]  Error: not a MemNode nor a VarNode\n");
					return true;
				}
			}
		}
	}
	isDep[N] = false;
	return false;
}

bool TFA::runOnModule(Module &M) {
	if (Input.getValue() == true) {
		InputValues &IV = getAnalysis<InputValues> ();
		inputDepValues = IV.getInputDepValues();
	}
	else {
		InputDep &IV = getAnalysis<InputDep> ();
		inputDepValues = IV.getInputDepValues();
	}
	AddStore &AS = getAnalysis<AddStore> ();
	depGraph = AS.getModifiedGraph();
	for (Module::iterator F = M.begin(), endF = M.end(); F != endF; ++F) {
		for (Function::iterator BB = F->begin(), endBB = F->end(); BB != endBB; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), endI = BB->end(); I
					!= endI; ++I) {
				if (AllocaInst* AI = dyn_cast<AllocaInst>(I)) {
					Type* Ty = AI->getAllocatedType();
					if (Ty->isArrayTy()) {
						if (isValueInpDep(AI)) {
							LLVMContext& C = AI->getContext();
							MDNode* N = MDNode::get(C,
									MDString::get(C, "TFA"));
							AI->setMetadata("tainted", N);
						}
					} else if (GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(I)) {
						if (PointerType* PO = dyn_cast<PointerType>(GEP->getType())) {
							if (PO->getElementType()->isArrayTy()) {
								while (isa<GetElementPtrInst> (
										GEP->getPointerOperand())) {
									GEP = cast<GetElementPtrInst> (
											GEP->getPointerOperand());
								}
								if (AllocaInst *AI = dyn_cast<AllocaInst>(GEP->getPointerOperand())) {
									LLVMContext& C = AI->getContext();
									MDNode* N = MDNode::get(C,
											MDString::get(C, "TFA"));
									AI->setMetadata("tainted", N);
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

void TFA::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<AddStore> ();
	AU.addRequired<InputValues> ();
	AU.addRequired<InputDep> ();
}

char TFA::ID = 0;
static RegisterPass<TFA> X("tfa", "Tainted Flow Analysis");
