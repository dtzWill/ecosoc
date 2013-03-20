
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
#include "../AliasSets/AliasSets.h"
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
	 * Class GraphNode
	 *
	 * This abstract class can do everything a simple graph node can do:
	 * 		- It knows the nodes that points to it
	 * 		- It knows the nodes who are ponted by it
	 * 		- It has a unique ID that can be used to identify the node
	 * 		- It knows how to connect itself to another GraphNode
	 *
	 * This class provides virtual methods that makes possible printing the graph
	 * in a fancy .dot file, providing for each node:
	 * 		- Label
	 * 		- Shape
	 * 		- Style
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
		virtual std::string getStyle();

		virtual GraphNode* clone() = 0;
	};

	/*
	 * Class OpNode
	 *
	 * This class represents the operation nodes:
	 * 		- It has a OpCode that is compatible with llvm::Instruction OpCodes
	 * 		- It may or may not store a value, that is the variable defined by the operation
	 */
	class OpNode: public GraphNode {
	private:
		unsigned int OpCode;
		Value* value;
	public:
		OpNode(int OpCode): GraphNode(), OpCode(OpCode), value(NULL) {this->Class_ID = 1;};
		OpNode(int OpCode, Value* v): GraphNode(), OpCode(OpCode), value(v) {this->Class_ID = 1;};
		static inline bool classof(const GraphNode *N) {return N->getClass_Id()==1 || N->getClass_Id()==3;};
		unsigned int getOpCode() const;
		void setOpCode(unsigned int opCode);
		Value* getValue();

		std::string getLabel();
		std::string getShape();

		GraphNode* clone();
	};

	/*
	 * Class CallNode
	 *
	 * This class represents operation nodes of llvm::Call instructions:
	 * 		- It stores the pointer to the called function
	 */
	class CallNode: public OpNode {
	private:
		Function* F;
	public:
		CallNode(CallInst* CI): OpNode(Instruction::Call, CI), F(CI->getCalledFunction()) {this->Class_ID = 3;};
		static inline bool classof(const GraphNode *N) {return N->getClass_Id()==3;};
		Function* getCalledFunction() const;

		std::string getLabel();
		std::string getShape();

		GraphNode* clone();
	};

	/*
	 * Class VarNode
	 *
	 * This class represents variables and constants which are not pointers:
	 * 		- It stores the pointer to the corresponding Value*
	 */
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


	/*
	 * Class VarNode
	 *
	 * This class represents AliasSets of pointer values:
	 * 		- It stores the ID of the AliasSet
	 * 		- It provides a method to get access to all the Values contained in the AliasSet
	 */
	class MemNode: public GraphNode {
	private:
		int aliasSetID;
		AliasSets *AS;
	public:
		MemNode(int aliasSetID, AliasSets *AS): aliasSetID(aliasSetID), AS(AS) {this->Class_ID = 4;};
		static inline bool classof(const GraphNode *N) {return N->getClass_Id()==4;};
		std::set<Value*> getAliases();
		static inline bool classof(const MemNode *N) {return true;};

		std::string getLabel();
		std::string getShape();
		GraphNode* clone();
		std::string getStyle();

		int getAliasSetId() const;
	};

	/*
	 * Class Graph
	 *
	 * Stores a set of nodes. Each node knows how to go to other nodes.
	 *
	 * The class provides methods to:
	 * 		- Find specific nodes
	 * 		- Delete specific nodes
	 * 		- Print the graph
	 *
	 */
	//Dependence Graph
	class Graph {
		private:

			std::set<GraphNode*> nodes;

			AliasSets *AS;

			void dfsVisit (GraphNode* u, std::set<GraphNode*> &visitedNodes); //Used by findConnectingSubgraph() method
			void dfsVisitBack (GraphNode* u, std::set<GraphNode*> &visitedNodes); //Used by findConnectingSubgraph() method
			bool isValidInst(Value *v); //Return true if the instruction is valid for dependence graph construction
			bool isMemoryPointer(Value *v); //Return true if the value is a memory pointer

		public:
			Graph(AliasSets *AS): AS(AS) {}; //Constructor
			~Graph (); //Destructor - Free adjacent matrix's memory
			GraphNode* addInst (Value *v); //Add an instruction into Dependence Graph
			void addEdge (GraphNode* src, GraphNode* dst);
			GraphNode* findNode(Value *op);  //Return the pointer to the node or NULL if it is not in the graph
			OpNode* findOpNode(Value *op);  //Return the pointer to the node or NULL if it is not in the graph

			//print graph in dot format
			void toDot (std::string s); //print in stdErr
			void toDot (std::string s, std::string fileName); //print in a file
			void toDot (std::string s, raw_ostream *stream); //print in any stream

			Graph generateSubGraph (Value *src, Value *dst); //Take a source value and a destination value and find a Connecting Subgraph from source to destination


			void deleteCallNodes(Function* F);

			void print();

	};



	/*
	 * Class functionDepGraph
	 *
	 * Function pass that provides an intraprocedural dependency graph
	 *
	 */
	class functionDepGraph : public FunctionPass {
	public:
        	static char ID; // Pass identification, replacement for typeid.
        	functionDepGraph() : FunctionPass(ID), depGraph(NULL) {}
        	void getAnalysisUsage(AnalysisUsage &AU) const;
        	bool runOnFunction(Function&);

        	Graph* depGraph;
	};


	/*
	 * Class moduleDepGraph
	 *
	 * Module pass that provides a context-insensitive interprocedural dependency graph
	 *
	 */
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
