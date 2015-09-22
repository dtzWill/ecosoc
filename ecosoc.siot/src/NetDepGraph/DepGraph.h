#ifndef DEPGRAPH_H_
#define DEPGRAPH_H_

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "depgraph"
#endif

#define USE_ALIAS_SETS true
#define USE_ELEVATOR true 

#ifdef USE_ELEVATOR
#include "../NetLevel/NetLevel.h"
#endif

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/GraphWriter.h"
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
#include <fstream>
#include "InputDep.h"
#include "InputValues.h"
//#include <boost/lexical_cast.hpp>

using namespace std;

namespace llvm {
STATISTIC(NrOpNodes, "Number of operation nodes");
STATISTIC(NrVarNodes, "Number of variable nodes");
STATISTIC(NrMemNodes, "Number of memory nodes");
STATISTIC(NrEdges, "Number of edges");
//STATISTIC(NetDeps, "Number network deps");

typedef enum {
    etData = 0, etControl = 1
} edgeType;

/*
 * Class GraphNode
 *
 * This abstract class can do everything a simple graph node can do:
 *              - It knows the nodes that points to it
 *              - It knows the nodes who are ponted by it
 *              - It has a unique ID that can be used to identify the node
 *              - It knows how to connect itself to another GraphNode
 *
 * This class provides virtual methods that makes possible printing the graph
 * in a fancy .dot file, providing for each node:
 *              - Label
 *              - Shape
 *              - Style
 *
 */
class GraphNode {
private:
    std::map<GraphNode*, edgeType> successors;
    std::map<GraphNode*, edgeType> predecessors;

    static int currentID;

protected:
    int Class_ID;
    int ID;
    int ProgramID; // netdepgraph : used to define which program this node belongs
    bool NetDep;   //netdepgraph : used to define that there is path from this node to some NetReadNode
    int guider;     // netdpegraph: used to mark nodes that belongs to a focus path
    bool Array ;
    bool VulArray ;
    bool CrossVulArray;
    std::string functionName;
    std::string Label;
    std::string Shape;
    std::string Style;
    std::string Name;
    std::string Color;
    bool visible;
    bool focus  ;
    int count;

    unsigned CodeLine;
    std::string CodeName;
    std::set<int> Levels;

public:
    GraphNode();
    
    GraphNode(GraphNode &G);

    virtual ~GraphNode();

    static inline bool classof(const GraphNode *N) {
        return true;
    }
    ;
    std::map<GraphNode*, edgeType> getSuccessors();
    bool hasSuccessor(GraphNode* succ);

    std::map<GraphNode*, edgeType> getPredecessors();
    bool hasPredecessor(GraphNode* pred);

    void connect(GraphNode* dst, edgeType type = etData);
    int getClass_Id() const;
    int getId() const;
    std::string getName();
    virtual std::string getLabel() = 0;
    virtual std::string getShape() = 0;
    virtual std::string getStyle();


    void setClass_Id(int classId);
    void setId(int id);
    void setName(std::string name);
    void setLabel(std::string label);
    void setShape(std::string shape);
    void setStyle(std::string style);
    void setColor(std::string color);

    bool setNodeCodeLineAndName(Instruction *inst);
    unsigned getNodeCodeLine();
    std::string getNodeCodeName();

    void setNodeLevels(std::set< int > );
    std::set<int> getNodeLevels();

    bool hasLevelIntersection(std::set<int> ComparedNode);

    bool isFocus(){
        return this->focus;
    }
    void setFocus(bool focus){
        this->focus = focus;
    }

    std::string getColor(){
        if(focus){
            return this->Color;
        }else if(ProgramID == 1){
            return "cyan";
        }else if(ProgramID == 2){
            return "salmon";
        }
        return this->Color;
    }

    void setFunctionName(std::string name){
        this->functionName = name;
    }
    std::string getFunctionName(){
        return this->functionName;
    }
    int getProgramID(){
        return ProgramID;
    }
    void setProgramID(int programID){
        this->ProgramID = programID;
    }
    bool isNetDep(){
        return NetDep;
    }
    void setNetDep(bool netDep){
        this->NetDep = netDep;
    }

    bool isArray(){
        return Array;
    }
    void setArray(bool array){
        this->Array = array;
    }

    bool isVulArray(){
        return VulArray;
    }
    void setVulArray(bool vulArray){
        this->VulArray = vulArray;
    }
    bool isCrossVulArray(){
        return VulArray;
    }
    void setCrossVulArray(bool crossVulArray){
        this->VulArray = crossVulArray;
    }


    int getGuider(){
        return guider;
    }
    void setGuider(int guider){
        this->guider = guider;
    }
    bool isVisible(){//visible or not in dot graph
        return visible;
    }
    void setVisible(bool v){
        this->visible=v;
    }

    bool isNetWrite(){ //it will be overloaded in NetWriteNode
        return false;
    }
 
    void add(){ // increments the number references to this node
       this->count++;
    }
    int getCount(){
	return this->count;
    }
    virtual GraphNode* clone() = 0;
};

/*
 * Class OpNode
 *
 * This class represents the operation nodes:
 *              - It has a OpCode that is compatible with llvm::Instruction OpCodes
 *              - It may or may not store a value, that is the variable defined by the operation
 */
class OpNode: public GraphNode {
private:
    unsigned int OpCode;
    Value* value;
public:
    OpNode(){
        //FIXME: Serialize OpCode
        this->Class_ID = 1;
        NrOpNodes++;
	this->count = 1;
       
    }

    OpNode(int OpCode) :
        GraphNode(), OpCode(OpCode), value(NULL) {
        this->Class_ID = 1;
	this->count = 1;
        NrOpNodes++;
    }
    ;
    OpNode(int OpCode, Value* v) :
        GraphNode(), OpCode(OpCode), value(v) {
        this->Class_ID = 1;
	this->count = 1;
        NrOpNodes++;
    }
    ;
    ~OpNode() {
        NrOpNodes--;
    }
    ;
    static inline bool classof(const GraphNode *N) {
        return N->getClass_Id() == 1 || N->getClass_Id() == 3;
    }
    ;
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
 *              - It stores the pointer to the called function
 */
class CallNode: public OpNode {
private:
    CallInst* CI;
public:
    CallNode(){
        this->Class_ID = 3;
	this->count = 1;
    }

    CallNode(CallInst* CI) :
        OpNode(Instruction::Call, CI), CI(CI) {
        this->Class_ID = 3;
	this->count = 1;
    }
    ;
    static inline bool classof(const GraphNode *N) {
        return N->getClass_Id() == 3;
    }
    ;
    Function* getCalledFunction() const;

    std::string getLabel();
    std::string getShape();

    GraphNode* clone();
};

/*
 * Class VarNode
 *
 * This class represents variables and constants which are not pointers:
 *              - It stores the pointer to the corresponding Value*
 */
class VarNode: public GraphNode {
private:
    Value* value;
public:
    VarNode(){
        this->Class_ID = 2;
	this->count = 1;
        NrVarNodes++;
        value = NULL;
    }
    VarNode(Value* value) :
        GraphNode(), value(value) {
        this->Class_ID = 2;
	this->count = 1;
        if (isa<Instruction> (value)) {
        	Instruction *I =  dyn_cast<Instruction> (value); 
		this->setNodeCodeLineAndName(I);
	}
        NrVarNodes++;
    }
    ;
    ~VarNode() {
        NrVarNodes--;
    }
    static inline bool classof(const GraphNode *N) {
        return N->getClass_Id() == 2;
    }
    ;
    Value* getValue();

    std::string getLabel();
    std::string getShape();

    GraphNode* clone();
};

/*
 * Class VarNode
 *
 * This class represents AliasSets of pointer values:
 *              - It stores the ID of the AliasSet
 *              - It provides a method to get access to all the Values contained in the AliasSet
 */
class MemNode: public GraphNode {
private:
    int aliasSetID;
    AliasSets *AS;
    int access;
    bool crossTainted;
    bool netTainted;
    bool localTainted;
public:
    MemNode(){
        this->Class_ID = 4;
	this->count = 1;
	this->crossTainted = false;
	this->netTainted = false;
	this->localTainted = false;
        NrMemNodes++;
    }

    MemNode(int aliasSetID, AliasSets *AS) :
        aliasSetID(aliasSetID), AS(AS) {
        this->Class_ID = 4;
	this->count = 1;
	this->crossTainted = false;
	this->netTainted = false;
	this->localTainted = false;
        NrMemNodes++;
    }
    ;
    ~MemNode() {
        NrMemNodes--;
    }
    ;
    static inline bool classof(const GraphNode *N) {
        return N->getClass_Id() == 4;
    }
    ;
    std::set<Value*> getAliases();

    std::string getLabel();
    std::string getShape();
    GraphNode* clone();
    std::string getStyle();

    int getAliasSetId() const;
    void setLocalTainted(){
	this->localTainted = true;
    }
    bool isLocalTainted(){
	return localTainted;
    }
    void setCrossTainted(){
	this->crossTainted = true;
    }
    bool isCrossTainted(){
	return crossTainted;
    }
    void setNetTainted(){
	this->localTainted = true;
    }
    bool isNetTainted(){
	return netTainted;
    }
    
};
/*
* Class NetworkWriteNode
*
* This class represents operation nodes that access network to write
*/
class NetWriteNode: public OpNode {
private:
    CallInst* CI;

public:
    NetWriteNode(){ this->Class_ID = 5;Style="filled"; 
	 this->count = 1;
	 Levels.insert(1);
    };
    NetWriteNode(CallInst* CI): OpNode(Instruction::Call, CI), CI(CI) {this->Class_ID = 5;
	this->count = 1;
	Style="filled"; 
	Levels.insert(1);
    };
    static inline bool classof(const GraphNode *N) {return N->getClass_Id()==5;};
    Function* getCalledFunction() const;

    std::string getLabel();
    std::string getShape();

    GraphNode* clone();

};

/*
*
* Class NetworkReadNode
*
* This class represents operation nodes that access network to read
*/
class NetReadNode: public OpNode {
private:
    CallInst* CI;

public:
    NetReadNode(){ CI=NULL;this->Class_ID = 6;
	this->count = 1;
	Style="filled"; Levels.insert(1);
    };
    NetReadNode(CallInst* CI): OpNode(Instruction::Call, CI), CI(CI) {
        this->Class_ID = 6;
	this->count = 1;
	Style="filled"; Levels.insert(1);
    };
    NetReadNode(Instruction* I) {
        this->Class_ID = 6;
	this->count = 1;
	Style="filled"; 
	Levels.insert(1);
    };
    static inline bool classof(const GraphNode *N) {return N->getClass_Id()==6;};
    Function* getCalledFunction() const;

    std::string getLabel();
    std::string getShape();

    GraphNode* clone();
};

class InputNode: public GraphNode {
private:
	std::string inputFunctionName;
	Input *i;
	GraphNode * memNode;

public:
	InputNode(Input* i, GraphNode* m, std::string name){
		this->Class_ID = 7;Style="filled";
		this->i = i;
		this->memNode = m;
		this->inputFunctionName = name;
		this->count = 1;
		ProgramID = i->getProgramId();		
		Label = name;
	};
	static inline bool classof(const GraphNode *N) {return N->getClass_Id()==7;};

	std::string getLabel() ;
	void setShape(std::string shape);
	std::string getShape();

	GraphNode* clone();
};

/*
 * Class Graph
 *
 * Stores a set of nodes. Each node knows how to go to other nodes.
 *
 * The class provides methods to:
 *              - Find specific nodes
 *              - Delete specific nodes
 *              - Print the graph
 *
 */
//Dependence Graph
class Graph {
private:

    llvm::DenseMap<Value*, GraphNode*> opNodes;
    llvm::DenseMap<Value*, GraphNode*> callNodes;

    llvm::DenseMap<Value*, GraphNode*> varNodes1;
    llvm::DenseMap<Value*, GraphNode*> varNodes2;
    llvm::DenseMap<int, GraphNode*> memNodes;

    //DenseMap<std::pair<GraphNode*, GraphNode*>, std::vector<GraphNode*> > visitedNodesMap; //the first GraphNode is the source, the second GraphNode is the sink and the vector is the visited flow
    DenseMap<std::pair<GraphNode*, GraphNode*>, bool> visitedNodesMap; //the first GraphNode is the source, the second GraphNode is the sink and the vector is the visited flow

    std::set<GraphNode*> memNodes1;
    std::set<GraphNode*> memNodes2;

    std::set<GraphNode*> netRead;
    std::set<GraphNode*> netRead2;
    std::set<GraphNode*> netWrite;
    std::set<GraphNode*> netWrite2;
    std::set<GraphNode*> nodes;

    AliasSets *AS;
    NetLevel<Function*>* EL;

    bool isValidInst(Value *v); //Return true if the instruction is valid for dependence graph construction
    bool isMemoryPointer(Value *v); //Return true if the value is a memory pointer

public:
    Graph(AliasSets *AS, NetLevel<Function*>* EL) :
        AS(AS), EL(EL) {
        NrEdges = 0;
    }
    ; //Constructor
    ~Graph(); //Destructor - Free adjacent matrix's memory
    GraphNode* addInst(Value *v,Function *f); //Add an instruction into Dependence Graph

    void addEdge(int programID, GraphNode* src, GraphNode* dst, edgeType type = etData);

    GraphNode* findNode(Value *op, int programID); //Return the pointer to the node or NULL if it is not in the graph
    GraphNode* findNetNodeByCodeLineAndName(unsigned codeLine, std::string codeName, int Program);
    std::set<GraphNode*> findNodes(std::set<Value*> values, int programID);
    void setNodeLevelsFromFlowGraphInfo();

    OpNode* findOpNode(Value *op, int programID); //Return the pointer to the node or NULL if it is not in the graph

    std::set<GraphNode*> getNodes();

    std::set<GraphNode*> getMemNodes1(){
        return memNodes1;
    }

    std::set<GraphNode*> getMemNodes2(){
        return memNodes2;
    }
    std::set<GraphNode*> getNetReadNodes(int programID){
	if(programID==1) return netRead;
        else return netRead2;
    }
    std::set<GraphNode*> getNetWriteNodes(int programID){
	if(programID==1) return netWrite;
        else return netWrite2;
    }

    std::string getNodeColor(GraphNode *node);
    bool isSendOrRecvNode(GraphNode *node);
    bool isNodeP1(GraphNode *node);

    //print graph in dot format
    class Guider {
    public:
        Guider(Graph* graph);
        std::string getNodeAttrs(GraphNode* n);
        std::string getEdgeAttrs(GraphNode* u, GraphNode* v);
        void setNodeAttrs(GraphNode* n, std::string attrs);
        void setEdgeAttrs(GraphNode* u, GraphNode* v, std::string attrs);
        void clear();
    private:
        Graph* graph;
        DenseMap<GraphNode*, std::string> nodeAttrs;
        DenseMap<std::pair<GraphNode*, GraphNode*>, std::string> edgeAttrs;
    };
    void saveStatisticsToFile(std::string DaemonClient);
    void toDot(std::string s); //print in stdErr
    void toDot(std::string s, std::string fileName); //print in a file
    void toDot(std::string s, raw_ostream *stream); //print in any stream
    void toDot(std::string s, raw_ostream *stream, llvm::Graph::Guider* g);
    void toDotFocusSendRecv(std::string s, raw_ostream *stream, string DaemonClient); //print in any stream
    void toDotFocusSendRecvWithMemAndInputNodes(std::vector<std::vector<GraphNode*> > visitedNodesList, std::string s, raw_ostream *stream, string DaemonClient);
   
    void saveToFile(std::string fileName); //save the graph to file to restore after
    int loadFromFile(std::string fileName, Graph *depGraph); // load a file and create the graph


    Graph generateSubGraph(Value *src, int srcProgramID, Value *dst, int dstProgramID); //Take a source value and a destination value and find a Connecting Subgraph from source to destination
    Graph generateSubGraph(GraphNode *src, int srcProgramID, GraphNode *dst, int dstProgramID); //Take a source node and a destination node and find a Connecting Subgraph from source to destination

    void dfsVisit(GraphNode* u, std::set<GraphNode*> &visitedNodes); //Used by findConnectingSubgraph() method
    bool dfsVisit(GraphNode* u, GraphNode* stop,std::set<GraphNode*> &visitedNodes);//Do DFS and return true if stopNode was visited. Visited Nodes are inserted in visitedNodes set.
    bool dfsVisitVec(GraphNode* u, GraphNode* stop,std::vector<GraphNode*> &visitedNodes);//Do DFS and return true if stopNode was visited. Visited Nodes are inserted in visitedNodes vector.
    void dfsVisitBack(GraphNode* u, std::set<GraphNode*> &visitedNodes); //Used by findConnectingSubgraph() method

    void deleteCallNodes(Function* F);

    /*
     * Function getNearestDependence
     *
     * Given a sink, returns the nearest source in the graph and the distance to the nearest source
     */
    std::pair<GraphNode*, int> getNearestDependency(Value* sink, int sinkProgramID,
                                                    std::set<Value*> sources, int sourcesProgramID, bool skipMemoryNodes);

    /*
     * Function getEveryDependency
     *
     * Given a sink, returns shortest path to each source (if it exists)
     */
    //std::map<GraphNode*, std::vector<GraphNode*> > getEveryDependency(llvm::Value* sink, int sinkProgramID, std::set<llvm::Value*> sources, int sourcesProgramID,
    //                                                                  bool skipMemoryNodes, bool skipNetworkNodes);

    /*
     * Function getEveryDependency
     *
     * Given a sink, returns shortest path to each source (if it exists) ant counts de dependencies of Network
     */
    std::map<GraphNode*, std::vector<GraphNode*> > getEveryDependency(
            llvm::Value* sink, int sinkProgramID, std::set<llvm::Value*> sources, int sourcesProgramId, bool skipMemoryNodes, bool skipNetworkNodes, bool *netdep);


    int getNumOpNodes();
    int getNumCallNodes();
    int getNumMemNodes();
    int getNumVarNodes();
    int getNumDataEdges();
    int getNumControlEdges();
    int getNumEdges(edgeType type);
};

/*
 * Class functionDepGraph
 *
 * Function pass that provides an intraprocedural dependency graph
 *
 */
class functionDepGraph: public FunctionPass {
public:
    static char ID; // Pass identification, replacement for typeid.
    functionDepGraph() :
        FunctionPass(ID), depGraph(NULL) {
    }
    void getAnalysisUsage(AnalysisUsage &AU) const;
    bool runOnFunction(Function&);

    Graph* depGraph;
};

/*
 * Class netDepGraph
 *
 * Module pass that provides a context-insensitive interprocedural dependency graph
 *
 */
class netDepGraph: public ModulePass {
public:
    static char ID; // Pass identification, replacement for typeid.
    netDepGraph() :
        ModulePass(ID), depGraph(NULL) {
    }
    void getAnalysisUsage(AnalysisUsage &AU) const;
    bool runOnModule(Module&);

    void matchParametersAndReturnValues(Function &F);
    void deleteCallNodes(Function* F);

    Graph* depGraph;
};

class ViewModuleDepGraph: public ModulePass {
public:
    static char ID; // Pass identification, replacement for typeid.
    ViewModuleDepGraph() :
        ModulePass(ID) {
    }

    void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.addRequired<netDepGraph> ();
        AU.setPreservesAll();
    }

    bool runOnModule(Module& M) {

        netDepGraph& DepGraph = getAnalysis<netDepGraph> ();
        Graph *g = DepGraph.depGraph;

        std::string tmp = M.getModuleIdentifier();
        replace(tmp.begin(), tmp.end(), '\\', '_');

        std::string Filename = "/tmp/" + tmp + ".dot";

        //Print dependency graph (in dot format)
        g->toDot(M.getModuleIdentifier(), Filename);

        DisplayGraph(sys::Path(Filename), true, GraphProgram::DOT);

        return false;
    }
};
}

#endif //DEPGRAPH_H_
