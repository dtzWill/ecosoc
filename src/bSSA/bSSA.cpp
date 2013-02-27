#define DEBUG_TYPE "bssa"

#include "bSSA.h"

using namespace llvm;

//*********************************************************************************************************************************************************************
//																DEPENDENCE GRAPH API
//*********************************************************************************************************************************************************************
//
// Author: Bruno R. Silva
// Contact: brunors172@gmail.com
// Date: 25/02/2013
// Last update: 27/02/2013
// Project: e-CoSoc
// Institution: Computer Science department of Federal University of Minas Gerais
//
//*********************************************************************************************************************************************************************



//****************************************************** Node class's member functions
//Constructor
Node::Node (Value *v, int ori) {
	operand = v;
	origin = ori;
}

//Return the operand field
Value * Node::getOperand () {
	return operand;
}

//****************************************************** Graph class's member functions
//Constructor: Alloc and clean the adjacent matrix. I am using adjacent matrix, because memory is not a problem, but computation time is an important constraint
Graph::Graph (int numMaxInstructions) {

	size = numMaxInstructions;
	matrix = new int* [size];
	for( int i = 0 ; i < size ; i++ )
		matrix[i] = new int[size];

	//Cleaning matrix
	for (int i=0; i<size; i++) {
		for (int j=0; j<size; j++) {
			matrix[i][j] = 0; 
		}	
	}
}



//Take a connecting subgraph and print (dot Format)
void Graph::connectingSubgraphToDot (vector<Value *> v) {
	int op, op2;
	errs()<<"digraph \"Connecting Subgraph  \"{\n";
		errs()<< "label=\"Connecting Subgraph \";\n";
			for (unsigned int i=0; i<v.size(); i++) {
				op = findOperand(v[i]);
				for (unsigned int j=0; j<v.size(); j++) {
					op2 = findOperand(v[j]);
					if (matrix[op][op2] == 1) {
						if (!isa<Constant>(map[op]->getOperand())) {
							errs()<<"\"(Opcode "<<cast<Instruction>(map[op]->getOperand())->getOpcodeName()<<") ";
							errs()<<"%"<<map[op]->getOperand()->getName()<<"\"";
						}
						else
							errs()<<"\""<<(*map[op]->getOperand())<<"\"";

						errs()<<"->";

						if (!isa<Constant>(map[op2]->getOperand())) {
							errs()<<"\"(Opcode "<<cast<Instruction>(map[op2]->getOperand())->getOpcodeName()<<") ";
							errs()<<"%"<<map[op2]->getOperand()->getName()<<"\"";
						} else
							errs()<<"\""<<(*map[op2]->getOperand())<<"\"";
						errs()<<"\n";
					}

				}
			}
	errs()<<"}\n\n";

}


//Take a source Value and a destination Value and find a connecting subgraph from source to destination
vector<Value *> Graph::findConnectingSubgraph (Value *src, Value *dst) {
	int u, v;
	std::vector<Value *> s;

	//DFS from source
	//Init
	collor.clear();
	ancestral.clear();
	for (unsigned int u=0; u<map.size(); u++) {
		collor.push_back(0); //0 - not visited yet
		ancestral.push_back(-1); //none ancestral yet
	}
	u = findOperand(src);
	if (u != -1) {
		dfsVisit (u);
	}

	//DFS from Destination
	//Init
	collorBack.clear();
	ancestralBack.clear();
	for (unsigned int u=0; u<map.size(); u++) {
		collorBack.push_back(0); //0 - not visited yet
		ancestralBack.push_back(-1); //none ancestral yet
	}
	v = findOperand(dst);
	if (v != -1) {
		dfsVisitBack (v); //DFS from source
	}

	//Returning subgraph
	for (unsigned int u=0; u<map.size(); u++) {
		if (collor[u]!=0 && collorBack[u]!=0) {
			s.push_back(map[u]->getOperand());
		}
	}
	return s;
}

//Used by Graph::dfs
void Graph::dfsVisit (int u) {
	collor[u] = 1;
	for (unsigned int v=0; v<map.size(); v++) {
		if (adjacent(v, u)) {
			if (collor[v] == 0) {
				ancestral[v] = u;
				dfsVisit(v);
			}
		}
	}
	collor[u] = 2;
}

//Used by Graph::dfs
void Graph::dfsVisitBack (int u) {
	collorBack[u] = 1;
	for (unsigned int v=0; v<map.size(); v++) {
		if (adjacent(u, v)) {
			if (collorBack[v] == 0) {
				ancestralBack[v] = u;
				dfsVisitBack(v);
			}
		}
	}
	collorBack[u] = 2;
}

//Return true if v is adjacent to u
bool Graph::adjacent(int v, int u) {
	if (matrix[u][v] != 0)
		return true;
	else
		return false;
}





//Print the adjacent matrix  (.dot format). You must copy and past the printed string in a empty text file and open it in a dot program like Xdot in order to see the dependence graph.
void Graph::toDot (std::string s) {
	int i, j;


/* Prit internal map (for debug)
 * 	int k=0;
	for (vector<Node *>::iterator vit = map.begin(), vend = map.end(); vit != vend; ++vit) {
		Value *v = (*vit)->getOperand();
		Instruction *inst = dyn_cast<Instruction>(v);
		if (inst != NULL) {
			errs()<<"Function "<<inst->getParent()->getParent()->getName() << "  " << k << " ";
		}
		errs()<< *v;
		errs()<<"\n";
		k++;
	}
*/


	errs()<<"digraph \"DFG for \'"<< s <<"\' function \"{\n";
	errs()<< "label=\"DFG for \'"<< s <<"\' function\";\n";
		for (i=0; i<size; i++) {
			for (j=0; j<size; j++) {
				if (matrix[i][j]==1) {
					if (!isa<Constant>(map[i]->getOperand())) {
						errs()<<"\"(Opcode "<<cast<Instruction>(map[i]->getOperand())->getOpcodeName()<<") ";
						errs()<<"%"<<map[i]->getOperand()->getName()<<"\"";
					}
					else
						errs()<<"\""<<(*map[i]->getOperand())<<"\"";

					errs()<<"->";

					if (!isa<Constant>(map[j]->getOperand())) {
						errs()<<"\"(Opcode "<<cast<Instruction>(map[j]->getOperand())->getOpcodeName()<<") ";
						if (cast<Instruction>(map[j]->getOperand())->getOpcode() == Instruction::Store) {
							errs()<<"to address "<<cast<User>(map[j]->getOperand())->getOperand(1)->getName();
						}else {
							errs()<<"%"<<map[j]->getOperand()->getName()<<"\"";
						}
					} else
						errs()<<"\""<<(*map[j]->getOperand())<<"\"";
					errs()<<"\n";
				}
			}

		}
	errs()<<"}\n\n";





}

//Destructor: free the matrix's memory
Graph::~Graph () {
	for( int i = 0 ; i < size ; i++ ) {
		delete [] matrix[i] ;
	}
	delete [] matrix ;

}



void Graph::addInst (Value *v)  {
	if (isInstValid(v)) { //If is a data manipulator instruction
		if (this->findOperand(v) == -1) { //If it has not processed yet
			this->map.push_back(new Node(v,0));
			if (!isa<Constant>(v)) { //If is not a constant
				if (cast<Instruction>(v)->getOpcode() != Instruction::Call) {
					for (unsigned int i=0; i<cast<User>(v)->getNumOperands(); i++) {
						Value *v1 = cast<User>(v)->getOperand(i);
						this->addInst(v1);
					}
					for (unsigned int i=0; i<cast<User>(v)->getNumOperands(); i++) {
						//Include it in the adjacent matrix
						if (findOperand(cast<User>(v)->getOperand(i)) != -1) {
							matrix[findOperand(cast<User>(v)->getOperand(i))][findOperand(v)] = 1;
						}
					}
				}else {
					Value *v1 = cast<User>(v)->getOperand(0);
					this->addInst(v1);
					//Include it into adjacent matrix
					if (findOperand(cast<User>(v)->getOperand(0)) != -1) {
						matrix[findOperand(cast<User>(v)->getOperand(0))][findOperand(v)] = 1;
					}
				}
			}
		}else {
			return;
		}
	}else {
		return;
	}
}


//It verify if the instruction is valid for the dependence graph, i.e. just data manipulator instructions are important for dependence graph
bool Graph::isInstValid(Value *v) {
	if (isa<Instruction>(v))
		switch (cast<Instruction>(v)->getOpcode()) {
			//N operands instructions
			case Instruction::PHI:
			//3 operands instructions
			case Instruction::AtomicCmpXchg:
			case Instruction::AtomicRMW:
			//2 operands instructions
			case Instruction::Add:
			case Instruction::FAdd:
			case Instruction::Sub:
			case Instruction::FSub:
			case Instruction::Mul:
			case Instruction::FMul:
			case Instruction::FDiv:
			case Instruction::UDiv:
			case Instruction::SDiv:
			case Instruction::URem:
			case Instruction::SRem:
			case Instruction::FRem:
			case Instruction::Shl:
			case Instruction::LShr:
			case Instruction::AShr:
			case Instruction::And:
			case Instruction::Or:
			case Instruction::Xor:
			case Instruction::GetElementPtr:
			case Instruction::ExtractElement:
			case Instruction::ExtractValue:
			case Instruction::Select:
			//1 operand instruction
			case Instruction::Trunc:
			case Instruction::ZExt:
			case Instruction::SExt:
			case Instruction::Load:
			case Instruction::Store:
			case Instruction::Alloca:
			case Instruction::BitCast:
			case Instruction::PtrToInt:
			case Instruction::IntToPtr:
			case Instruction::FPToUI:
			case Instruction::FPToSI:
			case Instruction::SIToFP:
			case Instruction::UIToFP:
			case Instruction::Call:
				return true;
			default:
				return false;
		}
	else if (isa<Constant>(v)) {
			return true;
		}else {
			return false;
		}
}




//Return the internal map's index related to the operand. Internal map is just to make a relation between matrix adjacent indexes and real operands
//Return -1 if the operand is not inside map.
int Graph::findOperand (Value *op) {
	int i;
	i=0;
	for (vector<Node *>::iterator vit = map.begin(), vend = map.end(); vit != vend; ++vit) {
		if ((*vit)->getOperand() == op ) {
			return (i);
		}
		i++;
	}
	return (-1);
}


Value * Graph::getValue (unsigned int i) {
	if (i<map.size())
		return (map[i]->getOperand());
	else
		return NULL;
}







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
        AU.addRequiredTransitive<DominanceFrontier>();
        AU.addRequiredTransitive<DominatorTree>();
       
        // This pass modifies the program, but not the CFG
        AU.setPreservesCFG();
}

bool bSSA::runOnFunction(Function &F) {
	int instCount=0;
	Value *testSrc, *testDst;
	
       
        // Iterate over all Basic Blocks of the Function, calling a função para criar a tabela de predicados
        for (Function::iterator Fit = F.begin(), Fend = F.end(); Fit != Fend; ++Fit) {
        	instCount = instCount + bbInstCount(Fit); //Acumular o número de instruções de cada bloco básico. Será usado para alocar a matriz de adjacência
        	makeTable(Fit);
        }


    //Making dependency graph
	Graph *g = new Graph(instCount*2);
	//Insere instruções no Grafo
	for (Function::iterator Fit = F.begin(), Fend = F.end(); Fit != Fend; ++Fit) {
		for (BasicBlock::iterator Bit = Fit->begin(), Bend = Fit->end(); Bit != Bend; ++Bit) {
			g->addInst(Bit);
			//errs()<<Bit->getName()<<"\n";
		}
	}

	//Print dependency graph (in dot format)
	//g->toDot(F.getName());

	testSrc = g->getValue(0);
	testDst = g->getValue(12);
	g->findConnectingSubgraph(testSrc, testDst);
	g->connectingSubgraphToDot(g->findConnectingSubgraph(testSrc, testDst));

    return false;
}

//Conta o número de instruções de um Bloco Básico
int bSSA:: bbInstCount (BasicBlock *BB) {
	int count = 0;
	for (BasicBlock::iterator Bit = BB->begin(), Bend = BB->end(); Bit != Bend; ++Bit) {
		count ++;
	}
	return (count);
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




