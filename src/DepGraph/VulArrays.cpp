#include "VulArrays.h"
//STATISTIC(NumVulArrays, "The number of vulnerability-causing arrays");

VulArrays::VulArrays() :
	FunctionPass(ID) {
}

bool VulArrays::structHasArray(StructType* ST) {
	for (StructType::element_iterator stb = ST->element_begin(), ste =
			ST->element_end(); stb != ste; ++stb) {
		if (isa<ArrayType> (*stb)) {
			return true;
		}
	}
	return false;
}

//@AI: array allocation instruction
bool VulArrays::isValueInpDep(Value* V, std::set<Value*> inputDepValues) {
	//Firstly, check if array or alias is passed as parameter to "any" lib function
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
				if (inputDepValues.count(*ai)) {
					return true;
				}
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
					return true;
				}
			}
		}
	}
	return false;
}

bool VulArrays::structHasArray(const StructType* ST) {
	for (StructType::element_iterator I = ST->element_begin(), E =
			ST->element_end(); I != E; ++I) {
		if ((*I)->isArrayTy()) {
			return true;
		} else if (const StructType* st = dyn_cast<StructType>(*I)) {
			return structHasArray(st);
		}
	}
	return false;
}

bool VulArrays::runOnFunction(Function &F) {
	InputValues &IV = getAnalysis<InputValues> ();
	moduleDepGraph &m = getAnalysis<moduleDepGraph> ();
	depGraph = m.depGraph;
	std::set<Value*> inputDepValues = IV.getInputDepValues();
	std::set<AllocaInst*> arrays;
	for (Function::iterator BB = F.begin(), endBB = F.end(); BB != endBB; ++BB) {
		for (BasicBlock::iterator I = BB->begin(), endI = BB->end(); I != endI; ++I) {
			if (AllocaInst* AI = dyn_cast<AllocaInst>(I)) {
				Type* Ty = AI->getAllocatedType();
				if (Ty->isArrayTy()) {
					arrays.insert(AI);
				}
			} else if (GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(I)) {
				//				errs() << *GEP << "\n";
				if (isValueInpDep(GEP, inputDepValues)) {
					if (PointerType* PO = dyn_cast<PointerType>(GEP->getType())) {
						if (PO->getElementType()->isArrayTy()) {//até aqui tem quer ter sempre
							while (true) {
								if (PointerType* P = dyn_cast<PointerType>(GEP->getPointerOperandType())) {
									if (P->getElementType()->isStructTy()) {
										//										errs() << "found\n";
										depStructs1.insert(
												GEP->getPointerOperand());
										break;
									}
								} else if (GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(GEP->getPointerOperand())) {
									GEP = gep;
								} else {
									break;
								}
							}
						}
					}
				}
				//			} else if (BitCastInst* BC = dyn_cast<BitCastInst>(I)) {
				//				//				errs() << *BC <<"\n";
				//				if (PointerType* PO = dyn_cast<PointerType>(BC->getSrcTy())) {
				//					if (StructType* ST = dyn_cast<StructType>(PO->getElementType())) {
				//						if (structHasArray(ST)) {
				//							if (isValueInpDep(BC->getOperand(0), inputDepValues)) {
				//								//						 errs() << "found bitcast\n";
				//								depStructs2.insert(BC);
				//							}
				//						}
				//					}
				//				}
			}
		}
	}
	if (arrays.size() > 1) {
		for (std::set<AllocaInst*>::iterator i = arrays.begin(), e =
				arrays.end(); i != e; ++i) {
			if (isValueInpDep(cast<Value> (*i), inputDepValues)) {
				depArrays.insert(*i);
			}
		}
	}
//	printArrays(F);
	return false;
}

void VulArrays::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	AU.addRequired<moduleDepGraph> ();
	AU.addRequired<InputValues> ();
}

void VulArrays::printArrays(Function &F) {
	if (depArrays.size() == 0 && depStructs1.size() == 0 && depStructs2.size()
			== 0)
		return;
	errs() << "[VulArrays]  " << F.getName() << " function\n";
	errs() << "[VulArrays]   Arrays:\n";
	for (std::set<const AllocaInst*>::iterator i = depArrays.begin(), e =
			depArrays.end(); i != e; ++i) {
		errs() << "[VulArrays]    " << **i << "\n";
	}
	errs() << "[VulArrays]   Structs:\n";
	for (std::set<const Value*>::iterator i = depStructs1.begin(), e =
			depStructs1.end(); i != e; ++i) {
		errs() << "[VulArrays]    " << **i << "\n";
	}
	//	errs() << "[VulArrays]   Structs (bitcast):\n";
	//	for (std::set<const BitCastInst*>::iterator i = depStructs2.begin(), e =
	//			depStructs2.end(); i != e; ++i) {
	//		errs() << "[VulArrays]    " << **i << "\n";
	//	}
}

std::set<const AllocaInst*> VulArrays::getVulArrays() {
	return depArrays;
}

char VulArrays::ID = 0;
static RegisterPass<VulArrays> X("vul-arrays",
		"Vulnerability pass: identify arrays that cause vulnerabilities");
