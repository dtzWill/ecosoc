
#ifndef DEPGRAPH_H_
#define DEPGRAPH_H_

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/raw_ostream.h"
#include <deque>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <stdio.h>

using namespace std;

namespace llvm {

	/*
	 * Graph Node base class
	 *
	 *
	 */
	class GraphNode {
	private:
		std::set<GraphNode*> successors;
		std::set<GraphNode*> predecessors;

		static int currentID;
		int ID;

	protected:
		int Class_ID;
	public:
		GraphNode ();
		virtual ~GraphNode ();

		static inline bool classof(const GraphNode *N) {return true;};
		std::set<GraphNode*> getSuccessors();
		bool hasSuccessor(GraphNode* succ);

		std::set<GraphNode*> getPredecessors();
		bool hasPredecessor(GraphNode* pred);

		void connect(GraphNode* dst);
		int getClass_Id() const;
		int getId() const;
		std::string getName();
		virtual std::string getLabel() = 0;
		virtual std::string getShape() = 0;

		virtual GraphNode* clone() = 0;
	};

	class OpNode: public GraphNode {
	private:
		unsigned int OpCode;
	public:
		OpNode(int OpCode): GraphNode(), OpCode(OpCode) {this->Class_ID = 1;};
		static inline bool classof(const GraphNode *N) {return N->getClass_Id()==1;};
		unsigned int getOpCode() const;
		void setOpCode(unsigned int opCode);

		std::string getLabel();
		std::string getShape();

		GraphNode* clone();
	};

	class CallNode: public OpNode {
	private:
		Function* F;
	public:
		CallNode(Function* F): OpNode(Instruction::Call), F(F) {this->Class_ID = 3;};
		static inline bool classof(const GraphNode *N) {return N->getClass_Id()==3;};
		Function* getCalledFunction() const;

		std::string getLabel();
		std::string getShape();

		GraphNode* clone();
	};

	class VarNode: public GraphNode {
	private:
		Value* value;
	public:
		VarNode(Value* value): GraphNode(), value(value) {this->Class_ID = 2;};
		static inline bool classof(const GraphNode *N) {return N->getClass_Id()==2;};
		Value* getValue();

		std::string getLabel();
		std::string getShape();

		GraphNode* clone();
	};

	class MemNode: public VarNode {
	public:
		vector<Value*> getAliases();
		static inline bool classof(const MemNode *N) {return true;};

		//std::string getLabel();
	};


	//Dependence Graph
	class Graph {
		private:

			std::set<GraphNode*> nodes;

		public:
			Graph() {}; //Constructor
			~Graph (); //Destructor - Free adjacent matrix's memory
			GraphNode* addInst (Value *v); //Add an instruction into Dependence Graph
			void addEdge (GraphNode* src, GraphNode* dst);
			GraphNode* findNode (Value *op);  //Return the index of operand or -1 if it is not in the internal map

			//print graph in dot format
			void toDot (std::string s); //print in stdErr
			void toDot (std::string s, std::string fileName); //print in a file
			void toDot (std::string s, raw_ostream *stream); //print in any stream

			bool isValidInst(Value *v); //Return true if the instruction is valid for dependence graph construction

			Graph generateSubGraph (Value *src, Value *dst); //Take a source value and a destination value and find a Connecting Subgraph from source to destination
			void dfsVisit (GraphNode* u, std::set<GraphNode*> &visitedNodes); //Used by findConnectingSubgraph() method
			void dfsVisitBack (GraphNode* u, std::set<GraphNode*> &visitedNodes); //Used by findConnectingSubgraph() method

			void deleteCallNodes(Function* F);

			void print();

	};


	class functionDepGraph : public FunctionPass {
	public:
        	static char ID; // Pass identification, replacement for typeid.
        	functionDepGraph() : FunctionPass(ID), depGraph(NULL) {}
        	void getAnalysisUsage(AnalysisUsage &AU) const;
        	bool runOnFunction(Function&);

        	Graph* depGraph;
	};

	class moduleDepGraph : public ModulePass {
	public:
        	static char ID; // Pass identification, replacement for typeid.
        	moduleDepGraph() : ModulePass(ID), depGraph(NULL) {}
        	void getAnalysisUsage(AnalysisUsage &AU) const;
        	bool runOnModule(Module&);

        	void matchParametersAndReturnValues(Function &F);
        	void deleteCallNodes(Function* F);

        	Graph* depGraph;
	};

}

#endif //DEPGRAPH_H_
