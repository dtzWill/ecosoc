#define DEBUG_TYPE "bssa"

#include "bSSA.h"


using namespace llvm;


//*********************************************************************************************************************************************************************
//																DEPENDENCE GRAPH CLIENT
//*********************************************************************************************************************************************************************
//
// Author: Bruno R. Silva
// Contact: brunors172@gmail.com
// Date: 25/02/2013
// Last update: 25/02/2013
// Project: e-CoSoc (Intel and Computer Science department of Federal University of Minas Gerais)
//
//*********************************************************************************************************************************************************************

//Funções membro da classe Pred
//Inicializa estrutura de dados para armazenar um predicado e instruções que dependem dele.
Pred::Pred(ICmpInst *p) {
	predicado = p;
}

void Pred::insereInstrucao (Instruction *i) {
	instrucoes.push_back(i);
}


int Pred::getNumInstrucoes () {
	return (instrucoes.size());
}



//Funções membro da classe bSSA
void bSSA::getAnalysisUsage(AnalysisUsage &AU) const {

        AU.addRequired<functionDepGraph>();

        AU.addRequiredTransitive<DominatorTree>();
        AU.addRequiredTransitive<DominanceFrontier>();

        // This pass modifies the program, but not the CFG
        AU.setPreservesCFG();
}

bool bSSA::runOnFunction(Function &F) {

	functionDepGraph& DepGraph = getAnalysis<functionDepGraph>();

    //Getting dependency graph
	Graph *g = DepGraph.depGraph;

	std::string Filename = "/tmp/" + F.getName().str() + ".dot";

	//Print dependency graph (in dot format)
	g->toDot(F.getName(), Filename);


//	Value *src, *dst;
//
//
//	//Insert instructions in the graph
//	for (Function::iterator BBit = F.begin(), BBend = F.end(); BBit != BBend; ++BBit) {
//		for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; ++Iit) {
//			if ( Iit->getName().equals("add4") ) src = Iit;
//			if ( Iit->getName().equals("sub") ) dst = Iit;
//		}
//	}
//
//	Filename = "/tmp/TESTE" + F.getName().str() + ".dot";
//	g->generateSubGraph(src, dst).toDot(F.getName(), Filename);

	//testSrc = g->getValue(0);
	//testDst = g->getValue(12);
	//g->findConnectingSubgraph(testSrc, testDst);
	//g->connectingSubgraphToDot(g->findConnectingSubgraph(testSrc, testDst));

    return false;
}

void bSSA::makeTable (BasicBlock *BB) {

  	TerminatorInst *ti = BB->getTerminator();
       
        BranchInst *bi = NULL;
        if ((bi = dyn_cast<BranchInst>(ti))) { //Se é desvio
                if (bi->isConditional()) { //Se é desvio condicional
			//"Pega" a instrução de comparação que antecede o branch
                        Value *condition = bi->getCondition();
                        ICmpInst *comparison = dyn_cast<ICmpInst>(condition);
			
			//Pega os operandos (predicados) da instrução de comparação que antecede o desvio
                        if (comparison) {
				predicados.push_back(new Pred(comparison));				
				//errs() << *comparison << "\n";
                        } 
                }
        }       
}

char bSSA::ID = 0;
static RegisterPass<bSSA> X("bssa", "B Assignment Construction");




//Funções membro da classe bSSA
void bSSA_module::getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequired<moduleDepGraph>();

        // This pass modifies the program, but not the CFG
        AU.setPreservesCFG();
}

bool bSSA_module::runOnModule(Module &M) {

	moduleDepGraph& DepGraph = getAnalysis<moduleDepGraph>();

    //Getting dependency graph
	Graph *g = DepGraph.depGraph;

	std::string Filename = "/tmp/" + M.getModuleIdentifier() + ".dot";

	//Print dependency graph (in dot format)
	g->toDot(M.getModuleIdentifier(), Filename);

    return false;
}

char bSSA_module::ID = 0;
static RegisterPass<bSSA_module> Y("bssam", "B Assignment Construction (Module Pass)");











