#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include <deque>
#include <algorithm>
#include <vector>
#include <string>


using namespace std;

namespace llvm {
	class Pred {
		ICmpInst *predicado;
		std::vector<Instruction *> instrucoes;

	public:
		Pred(ICmpInst *p);
		void insereInstrucao (Instruction *i);
		int getNumInstrucoes ();
	};

	






	class Node {
		private:
			int origin; //0 for origin from datapath - 1 for origin from memory
			Value *operand;	
		public:
			Node (Value *v, int ori);
			Value *getOperand ();
	};		


	//Dependence Graph 
	class Graph {
		private:
			int **matrix; //Adjacent matrix
			int size; //Size of adjacent matrix (It must be provide by class's user)
			std::vector<Node *> map; //internal map used to make a relation between adjacent matrix's indexes / Operands

			std::vector<int> collor; //0 - white, 1 - gray, 2 - black
			std::vector<int> ancestral; //

			std::vector<int> collorBack; //0 - white, 1 - gray, 2 - black
			std::vector<int> ancestralBack; //
	
		public:
			Graph (int numMaxInstructions); //Constructor - Alloc  memory for store the adjacent matrix
			~Graph (); //Destructor - Free adjacent matrix's memory
			void addInst (Value *v); //Add an instruction into Dependence Graph
			int findOperand (Value *op);  //Return the index of operand or -1 if it is not in the internal map
			void toDot (std::string s); //Print graph  (dot format)
			bool isInstValid(Value *v); //Return true if the instruction is valid for dependence graph construction
			vector<Value *> findConnectingSubgraph (Value *src, Value *dst); //Take a source value and a destination value and find a Connecting Subgraph from source to destination, returning the respective vertices
			void dfsVisit (int u); //Used by findConnectingSubgraph() method
			void dfsVisitBack (int u); //Used by findConnectingSubgraph() method
			bool adjacent(int v, int u); //Return true if u->v (if vertex v is adjacent to vertex u)
			void connectingSubgraphToDot (vector<Value *> s); //Take a Connecting Subgraph and print (dot Format). You must provide the vector of vertices which is a return of Graph::findConnectinSubgraph() method
			Value * getValue (unsigned int i); //

	};









	class bSSA : public FunctionPass {
	public:
        	static char ID; // Pass identification, replacement for typeid.
        	bSSA() : FunctionPass(ID) {}
        	void getAnalysisUsage(AnalysisUsage &AU) const;
        	bool runOnFunction(Function&);
		int bbInstCount (BasicBlock *);

	private:
		std::vector<Pred *> predicados;
        	// Variables always live
		void makeTable (BasicBlock *b);
	};


}

