#define DEBUG_TYPE "bssa"

#include "bSSA.h"

using namespace llvm;

std::vector<BasicBlock *> ProcessedBB;

//Receive a predicate and include it on predicate attribute
Pred::Pred(Value *p) {
	predicate = p;
}

//Receive a instruction and include it on insts vector
void Pred::addInst (Instruction *i) {
	insts.push_back(i);
}

//Receive a function pointer and include it on funcs vector
void Pred::addFunc (Function *f) {
	funcs.push_back(f);
}


//Return the total instruction count
int Pred::getNumInstrucoes () {
	return (insts.size());
}

int Pred::getNumFunctions () {
	return (funcs.size());
}

//Return the predicate
Value *Pred::getPred() {
	return (predicate);
}

//Return the instruction stored on insts vector, pointed for parameter i
Instruction *Pred::getInst(int i) {
	if (i < (signed int)insts.size())
		return (insts[i]);
	else return NULL;
}

Function *Pred::getFunc(int i) {
	if (i < (signed int) funcs.size())
		return (funcs[i]);
	else return NULL;
}

//Return true of *op instruction is gated (if it is stored on insts vector) for the predicate
bool Pred::isGated (Instruction *op) {
	unsigned int i;

	for (i=0; i<insts.size(); i++) {
		if (op == insts[i])
			return true;
	}

	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//Passes which are used by bSSA pass
void bSSA::getAnalysisUsage(AnalysisUsage &AU) const {

		AU.addRequired<PostDominatorTree>();
		AU.addRequired<moduleDepGraph>();

        // This pass will modifies the program, but not the CFG
        AU.setPreservesCFG();

}


//For each module, this method is executed
bool bSSA::runOnModule(Module &M) {
		moduleDepGraph& DepGraph = getAnalysis<moduleDepGraph>();
		Function *F;
		//Getting dependency graph
		Graph *g = DepGraph.depGraph;

		//src stores the instructions which are source of secret information
		//dst stores the instructions  which are public output channel like printf()
		std::vector<Value *> src, dst;

		for (Module::iterator Mit = M.begin(), Mend = M.end(); Mit != Mend; ++Mit) {
			F = Mit;
			// Iterate over all Basic Blocks of the Function
			for (Function::iterator Fit = F->begin(), Fend = F->end(); Fit != Fend; ++Fit) {
				makeTable(Fit, F); //Creating in memory the table with predicates and gated instructions
			}
			
		}

		//Including control edges into dependence graph.
		incGraph (g);

		//Stats from dependence graph
		errs()<<"Var Nodes " << g->getNumVarNodes()<<" \n";
		errs()<<"Op Nodes " <<g->getNumOpNodes()<<" \n";
		errs()<<"Mem Nodes " <<g->getNumMemNodes()<<" \n";
		errs()<<"Data Edges " <<g->getNumDataEdges()<<" \n";
		errs()<<"Control Nodes " <<g->getNumControlEdges()<<" \n";



		//Interates on all source code in order to get the sources of address (secret information) and sinks (instructions like printf)
		for (Module::iterator F = M.begin(), eM = M.end(); F != eM; ++F) {
				for (Function::iterator BB = F->begin(), e = F->end(); BB != e; ++BB) {
					for (BasicBlock::iterator I = BB->begin(), ee = BB->end(); I != ee; ++I) {
						if (dyn_cast<Instruction>(I)->getOpcode()==Instruction::PtrToInt) {
							src.push_back(I);
						}else if (dyn_cast<Instruction>(I)->getOpcode()==Instruction::Alloca) {
							src.push_back(I);
						}

						//If is a function call
						if (CallInst *CI = dyn_cast<CallInst>(I)) {
							Function *Callee = CI->getCalledFunction();
							if (Callee) {
								StringRef Name = Callee->getName();
								//if is a print function
								if (Name.equals("printf")) {
									dst.push_back(I);
								}else if (Name.equals("fiprintf")) {
									  dst.push_back(I);
								}else if (Name.equals("fprintf")) {
									  dst.push_back(I);
								}else if (Name.equals("iprintf")) {
									  dst.push_back(I);
								}else if (Name.equals("vfprintf")) {
									  dst.push_back(I);
								}else if (Name.equals("vprintf")) {
									  dst.push_back(I);
								}else if (Name.equals("fputc")) {
									  dst.push_back(I);
								}else if (Name.equals("fputs")) {
									  dst.push_back(I);
								}else if (Name.equals("putc")) {
									  dst.push_back(I);
								}else if (Name.equals("putchar")) {
									  dst.push_back(I);
								}else if (Name.equals("puts")) {
									  dst.push_back(I);
								}else if (Name.equals("fwrite")) {
									  dst.push_back(I);
								}else if (Name.equals("pwrite")) {
									  dst.push_back(I);
								}else if (Name.equals("write")) {
									  dst.push_back(I);
								}

								//If is a source of address
								if (Name.equals("malloc")) {
									src.push_back(I);
								}else if (Name.equals("calloc")) {
									  src.push_back(I);
								}else if (Name.equals("realloc")) {
									  src.push_back(I);
								}else if (Name.equals("realloccf")) {
									  src.push_back(I);
								}if (Name.equals("valloc")) {
									  src.push_back(I);
								}
							}
						}
					}
				}
			}

		//Stats about sources and sinks
		errs()<<"Num Sources " <<src.size()<<"\n";
		errs()<<"Nun Sinks " <<dst.size()<<"\n";

		unsigned int countWarning=0;
		unsigned int totalControlEdges=0, totalDataEdges=0;
		unsigned int totalNodes=0;

		//Search leaks for each (source, sink). If there is a leak, include the tainted subgraph into set of tainted subgraphs.
		int c=0;
		for (unsigned int i=0; i<src.size(); i++) {
			for (unsigned int j=0; j<dst.size(); j++) {
				Graph subG = g->generateSubGraph(src[i], dst[j]);
				Graph::iterator gIt = subG.begin();
				Graph::iterator gIte = subG.end();
				if (gIt != gIte) {
					totalControlEdges += subG.getNumControlEdges();
					totalDataEdges +=subG.getNumDataEdges();
					countWarning++;
					totalNodes += subG.getNumOpNodes()+subG.getNumVarNodes()+subG.getNumMemNodes();
					ostringstream ss;
					ss << "/tmp/subgrafo" << c <<".dot"; c++;
					subG.toDot("SubGrafo", ss.str()); //Make one file .dot for each tainted subgraph.
				}

			}
		}


		//Conta quantas arestas do grafo original fazem parte de pelo menos 1 subgrafo contaminado
		errs()<<"Tainted Edges " <<g->getTaintedEdges()<<"\n";

		//Número de warnings
		errs()<<"Num warnings " <<countWarning<<"\n";

		//Tamanho médio do subgrafo vulnerável
		if (countWarning>0){
		//Quantidade média de arestas por subgrafo
			errs()<<"Average Edges per tainted subgraph " <<(totalControlEdges +totalDataEdges)/countWarning<<"\n";
		//Quantidade média de arestas de controle por subgrafo
			errs()<<"Average Control Edges per tainted subgraph " <<totalControlEdges/countWarning<<"\n";
		//Quantidade média de arestas de dados por subgrafo
			errs()<<"Average Data Edges per tainted subgraph " <<totalDataEdges/countWarning<<"\n";
		//Quantidade média de nós no subgrafo
			errs()<<"Average nodes per tainted subgraph " <<totalNodes/countWarning<<"\n";
		}
		//Número de nós do grafo original que fazem parte de pelo menos 1 subgrafo contaminado
		errs()<<"Tainted nodes  " <<g->getTaintedNodesSize()<<"\n";

		//Make the file .dot including the dependence graph
		//std::string Filename = "/tmp/grafo.dot";
        //Print dependency graph (in dot format);
        //g->toDot("Grafo", Filename);

        return false;
}





//Increase graph including control edges
void bSSA::incGraph (Graph *g) {
	unsigned int i;
    int j;



	//For all predicates in predicatesVector
	for (i=0; i<predicatesVector.size(); i++) {
		//Locates the predicate (icmp instrution) Localiza o predicado (instrução icmp) from the graph
		GraphNode *predNode = g->findNode(predicatesVector[i]->getPred());

		//For each predicate, iterates on the list of gated INSTRUCTIONS
		for (j=0; j<predicatesVector[i]->getNumInstrucoes(); j++) {
			GraphNode *instNode = g->findNode(predicatesVector[i]->getInst(j));
			if (predNode != NULL && instNode != NULL) {//If the instruction is on the graph, make a edge
				g->addEdge(predNode, instNode, etControl);
			}
		}


		//For each predicate, iterates on the list of gated FUNCTIONS
		for (j=0; j<predicatesVector[i]->getNumFunctions(); j++) {
			Function *F = predicatesVector[i]->getFunc(j);
			//For each function, iterates on its basic blocks
			for (Function::iterator Fit = F->begin(), Fend = F->end(); Fit != Fend; ++Fit) {
				//For each basic block, iterates on its instructions
				for (BasicBlock::iterator bBIt = Fit->begin(), bBEnd = Fit->end(); bBIt != bBEnd; ++bBIt) {
					GraphNode *instNode = g->findNode(bBIt);
					if (predNode != NULL && instNode != NULL)
						g->addEdge(predNode, instNode, etControl);
				}
			}
		}

	}


}



//It receives a BasicBLock and makes table of predicates and its respective gated instructions
void bSSA::makeTable (BasicBlock *BB, Function *F) {
	    Value *condition;
  		TerminatorInst *ti = BB->getTerminator();
        BranchInst *bi = NULL;
        SwitchInst *si=NULL;

        PostDominatorTree &PD = getAnalysis<PostDominatorTree>(*F);

        ProcessedBB.clear();
        if ((bi = dyn_cast<BranchInst>(ti)) && bi->isConditional()) { //If the terminator instruction is a conditional branch
            condition = bi->getCondition();
            //Including the predicate on the predicatesVector
            predicatesVector.push_back(new Pred(condition));
            //Make a "Flooding" on each sucessor gated the instruction on Influence Region of the predicate
            for (unsigned int i=0; i<bi->getNumSuccessors(); i++) {
                 findIR (BB, bi->getSuccessor(i),PD);
            }
        }else if ((si = dyn_cast<SwitchInst>(ti))) {
        	condition = si->getCondition();
		    //Including the predicate on the predicatesVector
		    predicatesVector.push_back(new Pred(condition));
		    //Make a "Flooding" on each sucessor gated the instruction on Influence Region of the predicate
		    for (unsigned int i=0; i<si->getNumSuccessors(); i++) {
		        findIR (BB, si->getSuccessor(i),PD);
		    }
        }
}


//Flooding until reach a posdominator node
void bSSA::findIR (BasicBlock *bBOring, BasicBlock *bBSuss, PostDominatorTree &PD) {

	Pred *p;
	TerminatorInst *ti = bBSuss->getTerminator();


	//If the basic block has been processed, do not advance
	for (unsigned int x=0; x<ProcessedBB.size(); x++) {
		if (ProcessedBB[x] == bBSuss) {
			return;
		}
	}

	//Including the basic block in the processed vector
	ProcessedBB.push_back(bBSuss);

	//If the basic block is a posdominator and is not the start basic block, just gate the PHI instructions
	if (PD.dominates(bBSuss, bBOring) && bBSuss != bBOring) {
		//Find PHI instructions
		for (BasicBlock::iterator bBIt = bBSuss->begin(), bBEnd = bBSuss->end(); bBIt != bBEnd; ++bBIt) {
			if (dyn_cast<Instruction>(bBIt)->getOpcode()==Instruction::PHI) {
				//if there is a PHI's argument gated, gate the PHI instruction
				p = predicatesVector.back();
				for (unsigned int k=0; k<dyn_cast<Instruction>(bBIt)->getNumOperands(); k++) {
					if (p->isGated(dyn_cast<Instruction>(dyn_cast<Instruction>(bBIt)->getOperand(k)))) {
						p->addInst(bBIt);
						break;
					}
				}
			}
		}
		return;
	}else { //Advance the flooding
		//Instruction will be gated whit the bBOring predicate
		for (BasicBlock::iterator bBIt = bBSuss->begin(), bBEnd = bBSuss->end(); bBIt != bBEnd; ++bBIt) {
			p = predicatesVector.back();

			//If is a function call which is defined on the same module
			if (CallInst *CI = dyn_cast<CallInst>(&(*bBIt))) {
				Function *F = CI->getCalledFunction();
				if (F != NULL)
					if (!F->isDeclaration() && !F->isIntrinsic()) {
						gateFunction (F, p);
					}
			}

			//Gate the other instructions
			p->addInst(bBIt);
		}
		//If there is successor, go there
		for (unsigned int i=0; i<ti->getNumSuccessors(); i++) {
			findIR (bBOring, ti->getSuccessor(i), PD);
		}
	}

}

//All instrutions of function F are gated with predicate p
void bSSA::gateFunction (Function *F, Pred *p) {
	// Iterate over all Basic Blocks of the Function
	//for (Function::iterator Fit = F->begin(), Fend = F->end(); Fit != Fend; ++Fit) {
		//for (BasicBlock::iterator bBIt = Fit->begin(), bBEnd = Fit->end(); bBIt != bBEnd; ++bBIt) {
			p->addFunc(F);
		//}
	//}
}

//Print all predicates and its respective gated instructions
void bSSA::printGate () {
	for (unsigned int i=0; i<predicatesVector.size(); i++) {
		errs() << "\n\n"<<predicatesVector[i]->getPred()->getName()<<"\n";
		for (int j=0; j<predicatesVector[i]->getNumInstrucoes(); j++) {
			errs()<<predicatesVector[i]->getInst(j)->getOpcodeName()<<"\n";

		}
	}

}

char bSSA::ID = 0;
static RegisterPass<bSSA> X("bssa", "B Assignment Construction");




