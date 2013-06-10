#include "VulArrays.h"
//STATISTIC(NumVulArrays, "The number of vulnerability-causing arrays");

VulArrays::VulArrays() :
	ModulePass(ID) {
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

DenseMap<const Value*, std::vector<GraphNode*> > VulArrays::getValueDeps(
		Value* V, std::set<Value*> inputDepValues) {
	DenseMap<const Value*, std::vector<GraphNode*> > result;

	//Firstly, check if array or alias is passed as parameter to "any" lib function
	std::set<Value*> alias;
	std::vector<GraphNode*> aux;
	static DenseMap<GraphNode*,
			DenseMap<const Value*, std::vector<GraphNode*> > > Deps; // to avoid repeated computation
	GraphNode* N = depGraph->findNode(V);
	//	errs() << "--- ";
	//	errs() << N->getLabel() << "\n";
	if (N == NULL)
		return result;
	if (Deps.count(N)) {
		return Deps[N];
	}
	if (isa<MemNode> (N)) {
		MemNode *M = dyn_cast<MemNode> (N);
		alias = M->getAliases();
		for (std::set<Value*>::iterator ai = alias.begin(), aend = alias.end(); ai
				!= aend; ++ai) {
			if (inputDepValues.count(*ai)) {
				aux.push_back(depGraph->findNode(*ai));
				result[V] = aux;
				aux.clear();
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
				//				errs() << "Store inst found before ";
				//				errs() << *V << "\n";
				std::map<GraphNode*, std::vector<GraphNode*> > dep =
						depGraph->getEveryDependency(V, inputDepValues, false);
				if (dep.begin() != dep.end()) {
					// Get debug info
					if (Instruction* I = dyn_cast<Instruction>(ON->getValue())) {
						if (MDNode *mdn = I->getMetadata("dbg")) {
							DILocation Loc(mdn); // DILocation is in DebugInfo.h
							unsigned Line = Loc.getLineNumber();
							StringRef File = Loc.getFilename();
							debugInfo[std::make_pair<GraphNode*, GraphNode*>(n,
									N)]
									= std::make_pair<unsigned, std::string>(
											Line, File.str());
						}
					}
					for (std::map<GraphNode*, std::vector<GraphNode*> >::iterator
							ii = dep.begin(), ee = dep.end(); ii != ee; ++ii) {
						if (VarNode * VN = dyn_cast<VarNode> (ii->first)) {
							result[VN->getValue()] = ii->second;
						} else if (MemNode * MN = dyn_cast<MemNode> (ii->first)) {
							std::set<Value*>::iterator i =
									MN->getAliases().begin(); //get alias 0 as representative
							result[*i] = ii->second;
						}
						DEBUG(errs() << "[VulArrays]  Error: not a MemNode nor a VarNode\n");
					}
				}
			}
		}
	}
	Deps[N] = result;
	return result;
}

const Value* VulArrays::isValueInpDep(Value* V, std::set<Value*> inputDepValues) {
	//Firstly, check if array or alias is passed as parameter to "any" lib function
	std::set<Value*> alias;
	static DenseMap<GraphNode*, const Value*> isDep; // to avoid repeated computation
	GraphNode* N = depGraph->findNode(V);
	if (N == NULL)
		return NULL;
	if (isDep.count(N)) {
		return isDep[N];
	}
	if (isa<MemNode> (N)) {
		MemNode *M = dyn_cast<MemNode> (N);
		alias = M->getAliases();
		for (std::set<Value*>::iterator ai = alias.begin(), aend = alias.end(); ai
				!= aend; ++ai) {
			if (inputDepValues.count(*ai)) {
				isDep[N] = *ai;
				return *ai;
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
						return *i;
					}
					DEBUG(errs() << "[VulArrays]  Error: not a MemNode nor a VarNode\n");
					return V;
				}
			}
		}
	}
	isDep[N] = NULL;
	return NULL;
}

bool VulArrays::runOnModule(Module &M) {
	//	InputValues &IV = getAnalysis<InputValues> ();
	InputDep &IV = getAnalysis<InputDep> ();
	AddStore &AS = getAnalysis<AddStore> ();
	//	moduleDepGraph &m = getAnalysis<moduleDepGraph> ();
	//	depGraph = m.depGraph;
	depGraph = AS.getModifiedGraph();
	std::set<Value*> inputDepValues = IV.getInputDepValues();
	for (Module::iterator F = M.begin(), endF = M.end(); F != endF; ++F) {
		std::set<Value*> arrays;
		for (Function::iterator BB = F->begin(), endBB = F->end(); BB != endBB; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), endI = BB->end(); I
					!= endI; ++I) {
				if (AllocaInst* AI = dyn_cast<AllocaInst>(I)) {
					Type* Ty = AI->getAllocatedType();
					if (Ty->isArrayTy()) {
						arrays.insert(AI);
					}
				} else if (GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(I)) {
					if (PointerType* PO = dyn_cast<PointerType>(GEP->getType())) {
						if (PO->getElementType()->isArrayTy()) {
							bool IsStruct = false;
							while (isa<GetElementPtrInst> (
									GEP->getPointerOperand())) {
								IsStruct
										= IsStruct
												|| cast<PointerType> (
														GEP->getPointerOperandType())->getElementType()->isStructTy();
								GEP = cast<GetElementPtrInst> (
										GEP->getPointerOperand());
							}
							IsStruct
									= IsStruct
											|| cast<PointerType> (
													GEP->getPointerOperandType())->getElementType()->isStructTy();
							if (IsStruct) {
								//								errs() << "Is struct\n";
								if (AllocaInst *AI = dyn_cast<AllocaInst>(GEP->getPointerOperand())) {
									DenseMap<const Value*, std::vector<
											GraphNode*> > m = getValueDeps(GEP,
											inputDepValues);
									if (m.begin() != m.end()) {
										//										depStructs1[F].insert(
										//												std::make_pair(AI, v));
										Structs1[depGraph->findNode(AI)] = m;
									}
								}
							}
						}
					}
				} else if (BitCastInst* BC = dyn_cast<BitCastInst>(I)) {
					if (PointerType* PO = dyn_cast<PointerType>(BC->getSrcTy())) {
						if (StructType* ST = dyn_cast<StructType>(PO->getElementType())) {
							if (structHasArray(ST)) {
								DenseMap<const Value*, std::vector<GraphNode*> >
										m = getValueDeps(BC->getOperand(0),
												inputDepValues);
								if (m.begin() != m.end()) {
									Structs2[depGraph->findNode(
											BC->getOperand(0))] = m;
								}
							}
						}
					}
				}
			}
		}
		if (arrays.size() > 1) {
			for (std::set<Value*>::iterator i = arrays.begin(), e =
					arrays.end(); i != e; ++i) {
				DenseMap<const Value*, std::vector<GraphNode*> > m =
						getValueDeps(*i, inputDepValues);
				if (m.begin() != m.end()) {
					Arrays[depGraph->findNode(*i)] = m;
				}
			}
		}
	}
	toDot(M.getModuleIdentifier());
	//	printStats();
	//	printArrays();
	return false;
}

void VulArrays::toDot(std::string name) {
	Graph::Guider* guider = new Graph::Guider(depGraph);
	std::string ErrorInfo("");
	std::string fileName = name + ".vul.dot";
	std::string s;
	std::ostringstream ss;
	for (DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > >::iterator
			i = Arrays.begin(), e = Arrays.end(); i != e; ++i) {
		s = "[label=\"";
		s += i->first->getLabel() + "\" color=\"#FF6161\", style=filled]";
		guider->setNodeAttrs(i->first, s);
		DenseMap<const Value*, std::vector<GraphNode*> >::iterator ii =
				i->second.begin(), ee = i->second.end();
		if (Arrays.count(ii->second.front()))
			continue; // node should be red, not yellow
		s = "[label=\"";
		s += ii->second.front()->getLabel()
				+ "\" color=\"#FFFF7A\", style=filled]";
		guider->setNodeAttrs(ii->second.front(), s);
		GraphNode* u;
		GraphNode* v;
		while (ii != ee) {
			std::vector<GraphNode*>::iterator path = ii->second.begin(),
					endPath = ii->second.end();
			u = *path;
			while (path != endPath) {
				s = "[color=red fontsize=10";
				++path;
				v = *path;
				std::pair<GraphNode*, GraphNode*> uv = std::make_pair<
						GraphNode*, GraphNode*>(u, v);
				if (debugInfo.count(uv)) {
					ss << debugInfo[uv].first;
					s += " label=\"" + debugInfo[uv].second + ": line "
							+ ss.str() + "\"";
				}
				s += "]";
				guider->setEdgeAttrs(u, v, s);
				u = v;
			}
			++ii;
		}
	}
	for (DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > >::iterator
			i = Structs1.begin(), e = Structs1.end(); i != e; ++i) {
		s = "[label=\"";
		s += i->first->getLabel() + "\" color=\"#FF6161\", style=filled]";
		guider->setNodeAttrs(i->first, s);
		DenseMap<const Value*, std::vector<GraphNode*> >::iterator ii =
				i->second.begin(), ee = i->second.end();
		if (Structs1.count(ii->second.front()))
			continue; // node should be red, not yellow
		s = "[label=\"";
		s += ii->second.front()->getLabel()
				+ "\" color=\"#FFFF7A\", style=filled]";
		guider->setNodeAttrs(ii->second.front(), s);
		GraphNode* u;
		GraphNode* v;
		while (ii != ee) {
			std::vector<GraphNode*>::iterator path = ii->second.begin(),
					endPath = ii->second.end();
			u = *path;
			while (path != endPath) {
				s = "[color=red fontsize=10";
				++path;
				v = *path;
				std::pair<GraphNode*, GraphNode*> uv = std::make_pair<
						GraphNode*, GraphNode*>(u, v);
				if (debugInfo.count(uv)) {
					ss << debugInfo[uv].first;
					s += " label=\"" + debugInfo[uv].second + ": line "
							+ ss.str() + "\"";
				}
				s += "]";
				guider->setEdgeAttrs(u, v, s);
				u = v;
			}
			++ii;
		}
	}
	raw_fd_ostream File(fileName.c_str(), ErrorInfo);
	if (ErrorInfo.compare("")) {
		errs () << "[VulArrays]  " << ErrorInfo << "\n";
	}
	else
		depGraph->toDot(name, &File, guider);
}

//void VulArrays::printStats() {
//	int count;
//	double sum;
//	for (DenseMap<Value*, DenseMap<const Value*, std::vector<GraphNode*> > >::iterator
//			i = Arrays.begin(), e = Arrays.end(); i != e; ++i) {
//		errs() << "[VulArrays] " << *(i->first) << "\t";
//		count = 0;
//		sum = 0;
//		for (DenseMap<const Value*, std::vector<GraphNode*> >::iterator ii =
//				i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {
//			++count;
//			sum += ii->second.size() - 1;
//		}
//		errs() << sum / count << "\n";
//	}
//	errs() << "[VulArrays] ***********************************\n";
//	for (DenseMap<Value*, DenseMap<const Value*, std::vector<GraphNode*> > >::iterator
//			i = Structs1.begin(), e = Structs1.end(); i != e; ++i) {
//		errs() << "[VulArrays] " << *(i->first) << "\t";
//		count = 0;
//		sum = 0;
//		for (DenseMap<const Value*, std::vector<GraphNode*> >::iterator ii =
//				i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {
//			++count;
//			sum += ii->second.size() - 1;
//		}
//		errs() << sum / count << "\n";
//	}
//	errs() << "[VulArrays] ***********************************\n";
//	for (DenseMap<Value*, DenseMap<const Value*, std::vector<GraphNode*> > >::iterator
//			i = Structs2.begin(), e = Structs2.end(); i != e; ++i) {
//		errs() << "[VulArrays] " << *(i->first) << "\t";
//		count = 0;
//		sum = 0;
//		for (DenseMap<const Value*, std::vector<GraphNode*> >::iterator ii =
//				i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {
//			++count;
//			sum += ii->second.size() - 1;
//		}
//		errs() << sum / count << "\n";
//	}
//}

void VulArrays::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
	//	AU.addRequired<moduleDepGraph> ();
	AU.addRequired<AddStore> ();
	AU.addRequired<InputValues> ();
	AU.addRequired<InputDep> ();
}

char VulArrays::ID = 0;
static RegisterPass<VulArrays> X("vul-arrays",
		"Vulnerability pass: identify arrays that cause vulnerabilities");
