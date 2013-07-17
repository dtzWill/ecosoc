#include "DepGraph.h"

using namespace llvm;

static cl::opt<bool, false>
includeAllInstsInDepGraph("includeAllInstsInDepGraph", cl::desc("Include All Instructions In DepGraph."), cl::NotHidden);



//*********************************************************************************************************************************************************************
//                                                                                                                              DEPENDENCE GRAPH API
//*********************************************************************************************************************************************************************
//
// Author: Raphael E. Rodrigues
// Contact: raphaelernani@gmail.com
// Date: 11/03/2013
// Last update: 11/03/2013
// Project: e-CoSoc
// Institution: Computer Science department of Federal University of Minas Gerais
//
//*********************************************************************************************************************************************************************

//FIXME: Deal properly with invoke instructions. An Invoke instruction can be treated as a call node

/*
 * Class GraphNode
 */

GraphNode::GraphNode() {
        Class_ID = 0;
        ID = currentID++;
}

GraphNode::GraphNode(GraphNode &G) {
        Class_ID = 0;
        ID = currentID++;
}

GraphNode::~GraphNode() {

        for (std::map<GraphNode*, edgeType>::iterator pred = predecessors.begin(); pred
                        != predecessors.end(); pred++) {
                (*pred).first->successors.erase(this);
                NrEdges--;
        }

        for (std::map<GraphNode*, edgeType>::iterator succ = successors.begin(); succ
                        != successors.end(); succ++) {
                (*succ).first->predecessors.erase(this);
                NrEdges--;
        }

        successors.clear();
        predecessors.clear();
}

std::map<GraphNode*, edgeType> llvm::GraphNode::getSuccessors() {
        return successors;
}

std::map<GraphNode*, edgeType> llvm::GraphNode::getPredecessors() {
        return predecessors;
}

void llvm::GraphNode::connect(GraphNode* dst, edgeType type) {

        unsigned int curSize = this->successors.size();
        this->successors[dst] = type;
        dst->predecessors[this] = type;

        if (this->successors.size() != curSize) //Only count new edges
                NrEdges++;
}

int llvm::GraphNode::getClass_Id() const {
        return Class_ID;
}

int llvm::GraphNode::getId() const {
        return ID;
}

bool llvm::GraphNode::hasSuccessor(GraphNode* succ) {
        return successors.count(succ) > 0;
}

bool llvm::GraphNode::hasPredecessor(GraphNode* pred) {
        return predecessors.count(pred) > 0;
}

std::string llvm::GraphNode::getName() {
        std::ostringstream stringStream;
        stringStream << "node_" << getId();
        return stringStream.str();
}

std::string llvm::GraphNode::getStyle() {
        return std::string("solid");
}

int llvm::GraphNode::currentID = 0;

/*
 * Class OpNode
 */
unsigned int OpNode::getOpCode() const {
        return OpCode;
}

void OpNode::setOpCode(unsigned int opCode) {
        OpCode = opCode;
}

std::string llvm::OpNode::getLabel() {

        std::ostringstream stringStream;
        stringStream << Instruction::getOpcodeName(OpCode);
        return stringStream.str();

}

std::string llvm::OpNode::getShape() {
        return std::string("octagon");
}

GraphNode* llvm::OpNode::clone() {
        return new OpNode(*this);
}

llvm::Value* llvm::OpNode::getValue() {
        return value;
}

/*
 * Class CallNode
 */
Function* llvm::CallNode::getCalledFunction() const {
        return CI->getCalledFunction();
}

std::string llvm::CallNode::getLabel() {
        std::ostringstream stringStream;

        stringStream << "Call ";
        if (Function* F = getCalledFunction())
                stringStream << F->getName().str();
        else if (CI->hasName())
                stringStream << "*(" << CI->getName().str() << ")";
        else
                stringStream << "*(Unnamed)";

        return stringStream.str();
}

std::string llvm::CallNode::getShape() {
        return std::string("doubleoctagon");
}

GraphNode* llvm::CallNode::clone() {
        return new CallNode(*this);
}

/*
 * Class VarNode
 */
llvm::Value* VarNode::getValue() {
        return value;
}

std::string llvm::VarNode::getShape() {

        if (!isa<Constant> (value)) {
                return std::string("ellipse");
        } else {
                return std::string("box");
        }

}

std::string llvm::VarNode::getLabel() {

        std::ostringstream stringStream;

        if (!isa<Constant> (value)) {

                stringStream << value->getName().str();

        } else {

                if ( ConstantInt* CI = dyn_cast<ConstantInt>(value)) {
                        stringStream << CI->getValue().toString(10, true);
                } else {
                        stringStream << "Const:" << value->getName().str();
                }
        }

        return stringStream.str();

}

GraphNode* llvm::VarNode::clone() {
        return new VarNode(*this);
}

/*
 * Class MemNode
 */
std::set<llvm::Value*> llvm::MemNode::getAliases() {
        return USE_ALIAS_SETS ? AS->getValueSets()[aliasSetID] : std::set<
                        llvm::Value*>();
}

std::string llvm::MemNode::getLabel() {
        std::ostringstream stringStream;
        stringStream << "Memory " << aliasSetID;
        return stringStream.str();
}

std::string llvm::MemNode::getShape() {
        return std::string("ellipse");
}

GraphNode* llvm::MemNode::clone() {
        return new MemNode(*this);
}

std::string llvm::MemNode::getStyle() {
        return std::string("dashed");
}

int llvm::MemNode::getAliasSetId() const {
        return aliasSetID;
}

/*
 * Class Graph
 */
Graph::~Graph() {
        nodes.clear();
}

Graph Graph::generateSubGraph(Value *src, Value *dst) {
        Graph G(this->AS);

        std::map<GraphNode*, GraphNode*> nodeMap;

        std::set<GraphNode*> visitedNodes1;
        std::set<GraphNode*> visitedNodes2;

        GraphNode* source = findOpNode(src);
        if (!source) source = findNode(src);

        GraphNode* destination = findNode(dst);

        if (source == NULL || destination == NULL) {
                return G;
        }

        dfsVisit(source, visitedNodes1);
        dfsVisitBack(destination, visitedNodes2);

        //check the nodes visited in both directions
        for (std::set<GraphNode*>::iterator it = visitedNodes1.begin(); it
                        != visitedNodes1.end(); ++it) {
                if (visitedNodes2.count(*it) > 0) {
                        nodeMap[*it] = (*it)->clone();
                }
        }

        //connect the new vertices
        for (std::map<GraphNode*, GraphNode*>::iterator it = nodeMap.begin(); it
                        != nodeMap.end(); ++it) {

                std::map<GraphNode*, edgeType> succs = it->first->getSuccessors();

                for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
                                s_end = succs.end(); succ != s_end; succ++) {
                        if (nodeMap.count(succ->first) > 0) {

                                it->second->connect(nodeMap[succ->first], succ->second);

                        }
                }

                if ( ! G.nodes.count(it->second)) G.nodes.insert(it->second);

        }

        return G;
}

void Graph::dfsVisit(GraphNode* u, std::set<GraphNode*> &visitedNodes) {

        visitedNodes.insert(u);

        std::map<GraphNode*, edgeType> succs = u->getSuccessors();

        for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
                        succs.end(); succ != s_end; succ++) {
                if (visitedNodes.count(succ->first) == 0) {
                        dfsVisit(succ->first, visitedNodes);
                }
        }

}

void Graph::dfsVisitBack(GraphNode* u, std::set<GraphNode*> &visitedNodes) {

        visitedNodes.insert(u);

        std::map<GraphNode*, edgeType> preds = u->getPredecessors();

        for (std::map<GraphNode*, edgeType>::iterator pred = preds.begin(), s_end =
                        preds.end(); pred != s_end; pred++) {
                if (visitedNodes.count(pred->first) == 0) {
                        dfsVisitBack(pred->first, visitedNodes);
                }
        }

}

//Print the graph (.dot format) in the stderr stream.
void Graph::toDot(std::string s) {

        this->toDot(s, &errs());

}

void Graph::toDot(std::string s, const std::string fileName) {

        std::string ErrorInfo;

        raw_fd_ostream File(fileName.c_str(), ErrorInfo);

        if (!ErrorInfo.empty()) {
                errs() << "Error opening file " << fileName
                                << " for writing! Error Info: " << ErrorInfo << " \n";
                return;
        }

        this->toDot(s, &File);

}

void Graph::toDot(std::string s, raw_ostream *stream) {

        (*stream) << "digraph \"DFG for \'" << s << "\' function \"{\n";
        (*stream) << "label=\"DFG for \'" << s << "\' function\";\n";

        std::map<GraphNode*, int> DefinedNodes;

        for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
                        != end; node++) {

                if (DefinedNodes.count(*node) == 0) {
                        (*stream) << (*node)->getName() << "[shape=" << (*node)->getShape()
                                        << ",style=" << (*node)->getStyle() << ",label=\""
                                        << (*node)->getLabel() << "\"]\n";
                        DefinedNodes[*node] = 1;
                }

                std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();

                for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
                                s_end = succs.end(); succ != s_end; succ++) {

                        if (DefinedNodes.count(succ->first) == 0) {
                                (*stream) << (succ->first)->getName() << "[shape="
                                                << (succ->first)->getShape() << ",style="
                                                << (succ->first)->getStyle() << ",label=\""
                                                << (succ->first)->getLabel() << "\"]\n";
                                DefinedNodes[succ->first] = 1;
                        }

                        //Source
                        (*stream) << "\"" << (*node)->getName() << "\"";

                        (*stream) << "->";

                        //Destination
                        (*stream) << "\"" << (succ->first)->getName() << "\"";

                        if (succ->second == etControl)
                                (*stream) << " [style=dashed]";

                        (*stream) << "\n";

                }

        }

        (*stream) << "}\n\n";

}

void llvm::Graph::toDot(std::string s, raw_ostream *stream, llvm::Graph::Guider* g) {
        (*stream) << "digraph \"DFG for \'" << s << "\' module \"{\n";
        (*stream) << "label=\"DFG for \'" << s << "\' module\";\n";

        // print every node
        for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
                        != end; node++) {
                        (*stream) << (*node)->getName() << g->getNodeAttrs(*node) << "\n";

        }
        // print edges
        for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
                                != end; node++) {
                std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();
                for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
                                s_end = succs.end(); succ != s_end; succ++) {
                        //Source
                        (*stream) << "\"" << (*node)->getName() << "\"";
                        (*stream) << "->";
                        //Destination
                        (*stream) << "\"" << (succ->first)->getName() << "\"";
                        (*stream) << g->getEdgeAttrs(*node, succ->first);
                        (*stream) << "\n";
                }
        }
        (*stream) << "}\n\n";
}

GraphNode* Graph::addInst(Value *v) {

        GraphNode *Op, *Var, *Operand;

        CallInst* CI = dyn_cast<CallInst> (v);
        bool hasVarNode = true;

        if (isValidInst(v)) { //If is a data manipulator instruction
                Var = this->findNode(v);

                /*
                 * If Var is NULL, the value hasn't been processed yet, so we must process it
                 *
                 * However, if Var is a Pointer, maybe the memory node already exists but the
                 * operation node aren't in the graph, yet. Thus we must process it.
                 */
                if (Var == NULL || (Var != NULL && findOpNode(v) == NULL)) { //If it has not processed yet

                        //If Var isn't NULL, we won't create another node for it
                        if (Var == NULL) {

                                if (CI) {
                                        hasVarNode = !CI->getType()->isVoidTy();
                                }

                                if (hasVarNode) {
                                        if (StoreInst* SI = dyn_cast<StoreInst>(v))
                                                Var = addInst(SI->getOperand(1)); // We do this here because we want to represent the store instructions as a flow of information of a data to a memory node
                                        else if ((!isa<Constant> (v)) && isMemoryPointer(v)) {
                                                Var = new MemNode(
                                                                USE_ALIAS_SETS ? AS->getValueSetKey(v) : 0, AS);
                                                memNodes[USE_ALIAS_SETS ? AS->getValueSetKey(v) : 0]
                                                                = Var;
                                        } else {
                                                Var = new VarNode(v);
                                                varNodes[v] = Var;
                                        }
                                        nodes.insert(Var);
                                }

                        }

                        if (isa<Instruction> (v)) {

                                if (CI) {
                                        Op = new CallNode(CI);
                                        callNodes[CI] = Op;
                                } else {
                                        Op = new OpNode(dyn_cast<Instruction> (v)->getOpcode(), v);
                                }
                                opNodes[v] = Op;

                                nodes.insert(Op);
                                if (hasVarNode)
                                        Op->connect(Var);

                                //Connect the operands to the OpNode
                                for (unsigned int i = 0; i < cast<User> (v)->getNumOperands(); i++) {

                                        if (isa<StoreInst> (v) && i == 1)
                                                continue; // We do this here because we want to represent the store instructions as a flow of information of a data to a memory node

                                        Value *v1 = cast<User> (v)->getOperand(i);
                                        Operand = this->addInst(v1);

                                        if (Operand != NULL)
                                                Operand->connect(Op);
                                }
                        }
                }

                return Var;
        }
        return NULL;
}

void Graph::addEdge(GraphNode* src, GraphNode* dst, edgeType type) {

        nodes.insert(src);
        nodes.insert(dst);
        src->connect(dst, type);

}

//It verify if the instruction is valid for the dependence graph, i.e. just data manipulator instructions are important for dependence graph
bool Graph::isValidInst(Value *v) {

        if ((!includeAllInstsInDepGraph) && isa<Instruction> (v)) {

                //List of instructions that we don't want in the graph
                switch (cast<Instruction>(v)->getOpcode()) {

                        case Instruction::Br:
                        case Instruction::Switch:
                        case Instruction::Ret:
                                return false;

                }

        }

        if (v) return true;
        return false;

}

bool llvm::Graph::isMemoryPointer(llvm::Value* v) {
        if (v && v->getType())
                return v->getType()->isPointerTy();
        return false;
}

//Return the pointer to the node related to the operand.
//Return NULL if the operand is not inside map.
GraphNode* Graph::findNode(Value *op) {

        if ((!isa<Constant> (op)) && isMemoryPointer(op)) {
                int index = USE_ALIAS_SETS ? AS->getValueSetKey(op) : 0;
                if (memNodes.count(index))
                        return memNodes[index];
        } else {
                if (varNodes.count(op))
                        return varNodes[op];
        }

        return NULL;
}

std::set<GraphNode*> Graph::findNodes(std::set<Value*> values) {

        std::set<GraphNode*> result;

        for (std::set<Value*>::iterator i = values.begin(), end = values.end(); i
                        != end; i++) {

                if (GraphNode* node = findNode(*i)) {
                        result.insert(node);
                }

        }

        return result;
}

OpNode* llvm::Graph::findOpNode(llvm::Value* op) {

        if (opNodes.count(op))
                return dyn_cast<OpNode> (opNodes[op]);
        return NULL;
}

std::set<GraphNode*> llvm::Graph::getNodes() {
        return nodes;
}

void llvm::Graph::deleteCallNodes(Function* F) {

        for (Value::use_iterator UI = F->use_begin(), E = F->use_end(); UI != E; ++UI) {
                User *U = *UI;

                // Ignore blockaddress uses
                if (isa<BlockAddress> (U))
                        continue;

                // Used by a non-instruction, or not the callee of a function, do not
                // match.

                //FIXME: Deal properly with invoke instructions
                if (!isa<CallInst> (U))
                        continue;

                Instruction *caller = cast<Instruction> (U);

                if (callNodes.count(caller)) {
                        if (GraphNode* node = callNodes[caller]) {
                                nodes.erase(node);
                                delete node;
                        }
                        callNodes.erase(caller);
                }

        }

}

std::pair<GraphNode*, int> llvm::Graph::getNearestDependency(llvm::Value* sink,
                std::set<llvm::Value*> sources, bool skipMemoryNodes) {

        std::pair<llvm::GraphNode*, int> result;
        result.first = NULL;
        result.second = -1;

        if (GraphNode* startNode = findNode(sink)) {

                std::set<GraphNode*> sourceNodes = findNodes(sources);

                std::map<GraphNode*, int> nodeColor;

                std::list<std::pair<GraphNode*, int> > workList;

                for (std::set<GraphNode*>::iterator Nit = nodes.begin(), Nend =
                                nodes.end(); Nit != Nend; Nit++) {

                        if (skipMemoryNodes && isa<MemNode> (*Nit))
                                nodeColor[*Nit] = 1;
                        else
                                nodeColor[*Nit] = 0;
                }

                workList.push_back(pair<GraphNode*, int> (startNode, 0));

                /*
                 * we will do a breadth search on the predecessors of each node,
                 * until we find one of the sources. If we don't find any, then the
                 * sink doesn't depend on any source.
                 */

                while (workList.size()) {

                        GraphNode* workNode = workList.front().first;
                        int currentDistance = workList.front().second;

                        nodeColor[workNode] = 1;

                        workList.pop_front();

                        if (sourceNodes.count(workNode)) {

                                result.first = workNode;
                                result.second = currentDistance;
                                break;

                        }

                        std::map<GraphNode*, edgeType> preds = workNode->getPredecessors();

                        for (std::map<GraphNode*, edgeType>::iterator pred = preds.begin(),
                                        pend = preds.end(); pred != pend; pred++) {

                                if (nodeColor[pred->first] == 0) { // the node hasn't been processed yet

                                        nodeColor[pred->first] = 1;

                                        workList.push_back(
                                                        pair<GraphNode*, int> (pred->first,
                                                                        currentDistance + 1));

                                }

                        }

                }

        }

        return result;
}

std::map<GraphNode*, std::vector<GraphNode*> > llvm::Graph::getEveryDependency(
                llvm::Value* sink, std::set<llvm::Value*> sources, bool skipMemoryNodes) {

        std::map<llvm::GraphNode*, std::vector<GraphNode*> > result;
        DenseMap<GraphNode*, GraphNode*> parent;
        std::vector<GraphNode*> path;

        //      errs() << "--- Get every dep --- \n";
        if (GraphNode* startNode = findNode(sink)) {
                //              errs() << "found sink\n";
//                      errs() << "Starting search from " << startNode->getLabel() << "\n";
                std::set<GraphNode*> sourceNodes = findNodes(sources);
                std::map<GraphNode*, int> nodeColor;
                std::list<GraphNode*> workList;
                //              int size = 0;
                for (std::set<GraphNode*>::iterator Nit = nodes.begin(), Nend =
                                nodes.end(); Nit != Nend; Nit++) {
                        //                      size++;
                        if (skipMemoryNodes && isa<MemNode> (*Nit))
                                nodeColor[*Nit] = 1;
                        else
                                nodeColor[*Nit] = 0;
                }

                workList.push_back(startNode);
                nodeColor[startNode] = 1;
                /*
                 * we will do a breadth search on the predecessors of each node,
                 * until we find one of the sources. If we don't find any, then the
                 * sink doesn't depend on any source.
                 */
                //              int pb = 1;
                while (!workList.empty()) {
                        GraphNode* workNode = workList.front();
                        workList.pop_front();
                        if (sourceNodes.count(workNode)) {
                                //Retrieve path
                                path.clear();
                                GraphNode* n = workNode;
                                path.push_back(n);
                                while (parent.count(n)) {
                                        path.push_back(parent[n]);
                                        n = parent[n];
                                }
//                                std::reverse(path.begin(), path.end());
                                //                              errs() << "Path: ";
                                //                              for (std::vector<GraphNode*>::iterator i = path.begin(), e = path.end(); i != e; ++i) {
                                //                                      errs() << (*i)->getLabel() << " | ";
                                //                              }
                                //                              errs() << "\n";
                                result[workNode] = path;
                        }
                        std::map<GraphNode*, edgeType> preds = workNode->getPredecessors();
                        for (std::map<GraphNode*, edgeType>::iterator pred = preds.begin(),
                                        pend = preds.end(); pred != pend; pred++) {
                                if (nodeColor[pred->first] == 0) { // the node hasn't been processed yet
                                        nodeColor[pred->first] = 1;
                                        workList.push_back(pred->first);
                                        //                                      pb++;
                                        parent[pred->first] = workNode;
                                }
                        }
                        //                      errs() << pb << "/" << size << "\n";
                }
        }
        return result;
}


int llvm::Graph::getNumOpNodes() {
        return opNodes.size();
}

int llvm::Graph::getNumCallNodes() {
        return callNodes.size();
}

int llvm::Graph::getNumMemNodes() {
        return memNodes.size();
}

int llvm::Graph::getNumVarNodes() {
        return varNodes.size();
}

int llvm::Graph::getNumEdges(edgeType type){

        int result = 0;

    for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
                     != end; node++) {

             std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();

             for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
                             s_end = succs.end(); succ != s_end; succ++) {

                     if (succ->second == type) result++;

             }

     }

    return result;

}

int llvm::Graph::getNumDataEdges() {
        return getNumEdges(etData);
}

int llvm::Graph::getNumControlEdges() {
        return getNumEdges(etControl);
}


//*********************************************************************************************************************************************************************
//                                                                                                                              DEPENDENCE GRAPH CLIENT
//*********************************************************************************************************************************************************************
//vector
// Author: Raphael E. Rodrigues
// Contact: raphaelernani@gmail.com
// Date: 05/03/2013
// Last update: 05/03/2013
// Project: e-CoSoc (Intel and Computer Science department of Federal University of Minas Gerais)
//
//*********************************************************************************************************************************************************************


//Class functionDepGraph
void functionDepGraph::getAnalysisUsage(AnalysisUsage &AU) const {
        if (USE_ALIAS_SETS)
                AU.addRequired<AliasSets> ();

        AU.setPreservesAll();
}

bool functionDepGraph::runOnFunction(Function &F) {

        AliasSets* AS = NULL;

        if (USE_ALIAS_SETS)
                AS = &(getAnalysis<AliasSets> ());

        //Making dependency graph
        depGraph = new llvm::Graph(AS);
        //Insert instructions in the graph
        for (Function::iterator BBit = F.begin(), BBend = F.end(); BBit != BBend; ++BBit) {
                for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit
                                != Iend; ++Iit) {
                        depGraph->addInst(Iit);
                }
        }

        //We don't modify anything, so we must return false
        return false;
}

char functionDepGraph::ID = 0;
static RegisterPass<functionDepGraph> X("functionDepGraph",
                "Function Dependence Graph");

//Class moduleDepGraph
void moduleDepGraph::getAnalysisUsage(AnalysisUsage &AU) const {
        if (USE_ALIAS_SETS)
                AU.addRequired<AliasSets> ();

        AU.setPreservesAll();
}

bool moduleDepGraph::runOnModule(Module &M) {

        AliasSets* AS = NULL;

        if (USE_ALIAS_SETS)
                AS = &(getAnalysis<AliasSets> ());

        //Making dependency graph
        depGraph = new Graph(AS);

        //Insert instructions in the graph
        for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {
                for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit
                                != BBend; ++BBit) {
                        for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit
                                        != Iend; ++Iit) {
                                depGraph->addInst(Iit);
                        }
                }
        }

        //Connect formal and actual parameters and return values
        for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {

                // If the function is empty, do not do anything
                // Empty functions include externally linked ones (i.e. abort, printf, scanf, ...)
                if (Fit->begin() == Fit->end())
                        continue;

                matchParametersAndReturnValues(*Fit);

        }

        //We don't modify anything, so we must return false
        return false;
}

void moduleDepGraph::matchParametersAndReturnValues(Function &F) {

        // Only do the matching if F has any use
        if (F.isVarArg() || !F.hasNUsesOrMore(1)) {
                return;
        }

        // Data structure which contains the matches between formal and real parameters
        // First: formal parameter
        // Second: real parameter
        SmallVector<std::pair<GraphNode*, GraphNode*>, 4> Parameters(F.arg_size());

        // Fetch the function arguments (formal parameters) into the data structure
        Function::arg_iterator argptr;
        Function::arg_iterator e;
        unsigned i;

        //Create the PHI nodes for the formal parameters
        for (i = 0, argptr = F.arg_begin(), e = F.arg_end(); argptr != e; ++i, ++argptr) {

                OpNode* argPHI = new OpNode(Instruction::PHI);
                GraphNode* argNode = NULL;
                argNode = depGraph->addInst(argptr);

                if (argNode != NULL)
                        depGraph->addEdge(argPHI, argNode);

                Parameters[i].first = argPHI;
        }

        // Check if the function returns a supported value type. If not, no return value matching is done
        bool noReturn = F.getReturnType()->isVoidTy();

        // Creates the data structure which receives the return values of the function, if there is any
        SmallPtrSet<llvm::Value*, 8> ReturnValues;

        if (!noReturn) {
                // Iterate over the basic blocks to fetch all possible return values
                for (Function::iterator bb = F.begin(), bbend = F.end(); bb != bbend; ++bb) {
                        // Get the terminator instruction of the basic block and check if it's
                        // a return instruction: if it's not, continue to next basic block
                        Instruction *terminator = bb->getTerminator();

                        ReturnInst *RI = dyn_cast<ReturnInst> (terminator);

                        if (!RI)
                                continue;

                        // Get the return value and insert in the data structure
                        ReturnValues.insert(RI->getReturnValue());
                }
        }

        for (Value::use_iterator UI = F.use_begin(), E = F.use_end(); UI != E; ++UI) {
                User *U = *UI;

                // Ignore blockaddress uses
                if (isa<BlockAddress> (U))
                        continue;

                // Used by a non-instruction, or not the callee of a function, do not
                // match.
                if (!isa<CallInst> (U) && !isa<InvokeInst> (U))
                        continue;

                Instruction *caller = cast<Instruction> (U);

                CallSite CS(caller);
                if (!CS.isCallee(UI))
                        continue;

                // Iterate over the real parameters and put them in the data structure
                CallSite::arg_iterator AI;
                CallSite::arg_iterator EI;

                for (i = 0, AI = CS.arg_begin(), EI = CS.arg_end(); AI != EI; ++i, ++AI) {
                        Parameters[i].second = depGraph->addInst(*AI);
                }

                // Match formal and real parameters
                for (i = 0; i < Parameters.size(); ++i) {

                        depGraph->addEdge(Parameters[i].second, Parameters[i].first);
                }

                // Match return values
                if (!noReturn) {

                        OpNode* retPHI = new OpNode(Instruction::PHI);
                        GraphNode* callerNode = depGraph->addInst(caller);
                        depGraph->addEdge(retPHI, callerNode);

                        for (SmallPtrSetIterator<llvm::Value*> ri = ReturnValues.begin(),
                                        re = ReturnValues.end(); ri != re; ++ri) {
                                GraphNode* retNode = depGraph->addInst(*ri);
                                depGraph->addEdge(retNode, retPHI);
                        }

                }

                // Real parameters are cleaned before moving to the next use (for safety's sake)
                for (i = 0; i < Parameters.size(); ++i)
                        Parameters[i].second = NULL;
        }

        depGraph->deleteCallNodes(&F);
}

void llvm::moduleDepGraph::deleteCallNodes(Function* F) {
        depGraph->deleteCallNodes(F);
}

llvm::Graph::Guider::Guider(Graph* graph) {
        this->graph = graph;
        std::set<GraphNode*> nodes = graph->getNodes();
        for (std::set<GraphNode*>::iterator i = nodes.begin(), e = nodes.end(); i != e; ++i) {
                nodeAttrs[*i] = "[label=\"" + (*i)->getLabel() + "\" shape=\""+ (*i)->getShape() +"\" style=\""+ (*i)->getStyle()+"\"]";
        }
}

void llvm::Graph::Guider::setNodeAttrs(GraphNode* n, std::string attrs) {
        nodeAttrs[n] = attrs;
}

void llvm::Graph::Guider::setEdgeAttrs(GraphNode* u, GraphNode* v,
                std::string attrs) {
        edgeAttrs[std::make_pair<GraphNode*, GraphNode*>(u, v)] = attrs;
}

void llvm::Graph::Guider::clear() {
        nodeAttrs.clear();
        edgeAttrs.clear();
}

std::string llvm::Graph::Guider::getNodeAttrs(GraphNode* n) {
        if (nodeAttrs.count(n))
        return nodeAttrs[n];
        return "";
}

std::string llvm::Graph::Guider::getEdgeAttrs(GraphNode* u, GraphNode* v) {
        std::pair<GraphNode*, GraphNode*> edge = std::make_pair<GraphNode*, GraphNode*>(u, v);
        if (edgeAttrs.count(edge))
        return edgeAttrs[edge];
        return "";
}

char moduleDepGraph::ID = 0;
static RegisterPass<moduleDepGraph> Y("moduleDepGraph",
                "Module Dependence Graph");

char ViewModuleDepGraph::ID = 0;
static RegisterPass<ViewModuleDepGraph> Z("view-depgraph",
                "View Module Dependence Graph");
