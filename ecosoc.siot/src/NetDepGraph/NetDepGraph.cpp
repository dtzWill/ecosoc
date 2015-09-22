#include "DepGraph.h"
#include "../NetLevel/NetLevel.h"

STATISTIC(NumFunc, "The number of functions");
//STATISTIC(NumFuncArr, "The number of functions that contain arrays");
//STATISTIC(NumArr, "The number of arrays");
STATISTIC(NumInsts, "The number of instructions");
STATISTIC(NumStores, "The number of store insts");
STATISTIC(NumLoads, "The number of load");
STATISTIC(NumLoadsNotCount, "The number of loads to be fix");
STATISTIC(NumNodesP1, "Program #1: The number of nodes");
STATISTIC(NumSendNodesP1, "Program #1: The number of send nodes");
STATISTIC(NumRecvNodesP1, "Program #1: The number of receive nodes");
STATISTIC(NumSendRecvEdgesP1P2, "Program #1 & #2: The number of edges between send and receive");
STATISTIC(NumNodesP2, "Program #2: The number of nodes");
STATISTIC(NumSendNodesP2, "Program #2: The number of send nodes");
STATISTIC(NumRecvNodesP2, "Program #2: The number of receive nodes");

using namespace llvm;

static cl::opt<bool, false>
includeAllInstsInDepGraph("includeAllInstsInDepGraph", cl::desc("Include All Instructions In DepGraph."), cl::NotHidden);


//*********************************************************************************************************************************************************************
//                                                                                                                           NET   DEPENDENCE GRAPH API
//*********************************************************************************************************************************************************************
// Author Fernando Augusto Teixeira (teixeiracl@gmail.com)
// This code is an extension of DepGraph made by Raphael E. Rodrigues (raphaelernani@gmail.com) in june of 2013. 
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
    functionName="Unnamed";
    Color="black";
    visible=true;
    focus = false;
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

void llvm::GraphNode::setClass_Id( int class_ID) {
    Class_ID = class_ID;
}


int llvm::GraphNode::getId() const {
    return ID;
}

void llvm::GraphNode::setId(int id) {
    ID = id;
}

bool llvm::GraphNode::hasSuccessor(GraphNode* succ) {
    return successors.count(succ) > 0;
}

bool llvm::GraphNode::hasPredecessor(GraphNode* pred) {
    return predecessors.count(pred) > 0;
}

std::string llvm::GraphNode::getName() {
    std::ostringstream stringStream;
    stringStream << "node_" << getId() << "P"<<getProgramID();
    return stringStream.str();
}

std::string llvm::GraphNode::getStyle() {
    return std::string("solid");

}

int llvm::GraphNode::currentID = 0;

void llvm::GraphNode::setLabel(std::string label){
    Label = label;
}
void llvm::GraphNode::setName(std::string name){
    Name = name;
}
void llvm::GraphNode::setShape(std::string shape){
    Shape = shape;
}
void llvm::GraphNode::setStyle(std::string style){
    Style = style;
}

bool llvm::GraphNode::setNodeCodeLineAndName(Instruction *inst)
{
	MDNode *N = NULL;

	if (inst != NULL && (N = inst->getMetadata("dbg")))
	{
		DILocation Loc(N); // DILocation is in DebugInfo.h
		CodeLine = Loc.getLineNumber();
                CodeName = Loc.getDirectory().str() + '/' + Loc.getFilename().str();;
		return true;
	}

	return false;
}

unsigned llvm::GraphNode::getNodeCodeLine()
{
	return CodeLine;
}

std::string llvm::GraphNode::getNodeCodeName()
{
	return CodeName;
}

std::set<int> llvm::GraphNode::getNodeLevels()
{
	return Levels;
}

void llvm::GraphNode::setNodeLevels(std::set< int > p_Levels)
{
	Levels = p_Levels;
}

bool llvm::GraphNode::hasLevelIntersection(std::set<int> ComparedNode)
{
	std::set<int> NodeLevels = getNodeLevels();

    for (std::set<int>::iterator it = NodeLevels.begin(); it
         != NodeLevels.end(); ++it) {
    	if(ComparedNode.count(*it) > 0)
    		return true;
    }
    return false;
}

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
    //        if(Label.empty()){
    std::ostringstream stringStream;
    stringStream << Instruction::getOpcodeName(OpCode);
    Label =  stringStream.str();
    //        }
    return Label;
}

std::string llvm::OpNode::getShape() {
    if(Shape.empty()){
        Shape = std::string("octagon");
    }
    return Shape;
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
    //        if(Label.empty()){
    std::ostringstream stringStream;

    stringStream << "Call ";
    if (Function* F = getCalledFunction())
        stringStream << F->getName().str();
    else if (CI->hasName())
        stringStream << "*(" << CI->getName().str() << ")";
    else
        stringStream << "*(Unnamed)";
    Label = stringStream.str();
    //        }
    return Label;
}

std::string llvm::CallNode::getShape() {
    if(Shape.empty()){
        Shape = std::string("doubleoctagon");
    }
    return Shape;
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
    if(Shape.empty()){
        if (!isa<Constant> (value)) {
            Shape = std::string("ellipse");
        } else {
            Shape = std::string("box");
        }
    }
    return Shape;
}

std::string llvm::VarNode::getLabel() {
    //    if(Label.empty()) {
    std::ostringstream stringStream;
    /*		if(value == NULL or !value->hasName()){
            stringStream << "no value: node " <<getId();
         //       DEBUG(errs()<<"VarNode::getLabel -> Value without name\n");
        }
        else */if (!isa<Constant> (value)) {

        //      DEBUG(errs()<<"VarNode::getLabel -> !isa<Constant> (value) \n");
        stringStream << value->getName().str();

    } else {

        if ( ConstantInt* CI = dyn_cast<ConstantInt>(value)) {
            //     	DEBUG(errs()<<"VarNode::getLabel -> <ConstantInt> (value) \n");
            stringStream << CI->getValue().toString(10, true);
        } else {
            //    	DEBUG(errs()<<"VarNode::getLabel -> <Const>  \n");
            stringStream << "Const:" << value->getName().str();
        }
    }
    Label = stringStream.str();
    //   }
    return Label;

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
    //    if(Label.empty()){
    std::ostringstream stringStream;
    stringStream << "Memory " << aliasSetID;
//    if(Label == "INPUT " || Label == "INPUT " + stringStream.str())
//    	Label = "INPUT " + stringStream.str();
//    else
    	Label = stringStream.str();
    //   }else{
    //        std::ostringstream stringStream;
    //       stringStream << "p1";
    //      Label = stringStream.str();
    // }
    return Label;
}

std::string llvm::MemNode::getShape() {
    if(Shape.empty()){
        Shape = std::string("ellipse");
    }
    return Shape;
}

GraphNode* llvm::MemNode::clone() {
    return new MemNode(*this);
}

std::string llvm::MemNode::getStyle() {
    if(Style.empty()){
        Style = std::string("dashed");
    }
    return Style;
}

int llvm::MemNode::getAliasSetId() const {
    return aliasSetID;
}

/*
     * Class NetWriteNode
     */
static int contWrite=0;
Function* llvm::NetWriteNode::getCalledFunction() const {
    if(CI == NULL) return NULL;
    return CI->getCalledFunction();
}

std::string llvm::NetWriteNode::getLabel() {
    //     if(Label.empty()){
    std::ostringstream stringStream;

    Label = Label.empty() ? "Unnamed" : Label;

    stringStream << getName() << " Net Write: " <<  " Code line" << CodeLine << " " <<contWrite;
    contWrite++;

//    if(Function* F = getCalledFunction()){
//        stringStream << F->getName().str() << " " << contWrite;
//        contWrite++;
//    }
//    else if (CI != NULL && CI->hasName())
//        stringStream << "*(" << CI->getName().str() << ")";
//    else
//        stringStream << getFunctionName();

    return stringStream.str();
    //    }
    //     else return Label;
}


std::string llvm::NetWriteNode::getShape() {
    return std::string("hexagon");
}

GraphNode* llvm::NetWriteNode::clone() {
    return new NetWriteNode(*this);
}


/*
     * Class NetReadNode
     */
static int contRead=0;
static int contReadNoName=0;
Function* llvm::NetReadNode::getCalledFunction() const {
    if(CI == NULL) return NULL;
    return CI->getCalledFunction();
}

std::string llvm::NetReadNode::getLabel() {

    //     if(Label.empty()){
    std::ostringstream stringStream;

    Label = Label.empty() ? "Unnamed" : Label;

    stringStream << getName() << " Net Read: " << " Code line" << CodeLine << " " << contRead;
    contRead++;

//    if(Function* F = getCalledFunction()){
//        stringStream << F->getName().str() << " " << contRead;
//        contRead++;
//    }
//    else if (CI != NULL && CI->hasName())
//        stringStream << "*(" << CI->getName().str() << ")";
//    else
//    {
//        stringStream << Label << " " << contReadNoName;
//        contReadNoName++;
//    }

    return stringStream.str();
    //     }
    //     else return Label;
}

std::string llvm::NetReadNode::getShape() {
    return std::string("hexagon");
}

GraphNode* llvm::NetReadNode::clone() {
    return new NetReadNode(*this);
}

GraphNode* llvm::InputNode::clone() {
    return new InputNode(*this);
}

//Class InputNode
std::string llvm::InputNode::getLabel() {
	return Label;
}

std::string llvm::InputNode::getShape() {
	return Shape;
}

void llvm::InputNode::setShape(std::string shape) {
	Shape = shape;
}

/*
     * Class Graph
     */
Graph::~Graph() {
    nodes.clear();
}

Graph Graph::generateSubGraph(Value *src, int srcProgramID, Value *dst, int dstProgramID) {
    Graph G(this->AS, this->EL);

    GraphNode* source = findOpNode(src,srcProgramID);
    if (!source) source = findNode(src,srcProgramID);

    GraphNode* destination = findNode(dst,dstProgramID);

    if (source == NULL || destination == NULL) {
        return G;
    }
    G = generateSubGraph(source, srcProgramID, destination, dstProgramID) ;
    return G;
    }

void nodesInsert(std::set<GraphNode*> &nodes, GraphNode *node, int programId){
	std::pair<std::set<GraphNode*>::iterator,bool> ret;
      	ret = nodes.insert(node);
        if (ret.second==false) return;  // no new element inserted
	//DEBUG(errs()<<"Nodes Size="<<nodes.size()<<" node ["<< node->getName()<<"] label ["<<node->getLabel()<<"] set max size["<<nodes.max_size()<<"]\n");
	if(programId==1){
           NumNodesP1++;
        } else if(programId==2){
           NumNodesP2++;
        }
}
Graph Graph::generateSubGraph(GraphNode *source, int srcProgramID, GraphNode *destination, int dstProgramID) {
    Graph G(this->AS, this->EL);
    std::map<GraphNode*, GraphNode*> nodeMap;
    std::set<GraphNode*> visitedNodes1;
    std::set<GraphNode*> visitedNodes2;
    dfsVisit(source, visitedNodes1);
    dfsVisitBack(destination, visitedNodes2);

    //check the nodes visited in both directions
    DEBUG(errs()<<"Check the nodes visited in both directions\n");
    for (std::set<GraphNode*>::iterator it = visitedNodes1.begin(); it
         != visitedNodes1.end(); ++it) {
        if (visitedNodes2.count(*it) > 0) {
            nodeMap[*it] = (*it)->clone();
        }
    }

    //connect the new vertices
    DEBUG(errs()<<"connect the new vertices\n");
    for (std::map<GraphNode*, GraphNode*>::iterator it = nodeMap.begin(); it
         != nodeMap.end(); ++it) {

        std::map<GraphNode*, edgeType> succs = it->first->getSuccessors();

        for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
             s_end = succs.end(); succ != s_end; succ++) {
            if (nodeMap.count(succ->first) > 0) {

                it->second->connect(nodeMap[succ->first], succ->second);

            }
        }

        //if ( ! G.nodes.count(it->second)) nodesInsert(srcProgramID, G.nodes, *(it->second)); //FIXME

    }

    return G;
}

void Graph::dfsVisit(GraphNode* u, std::set<GraphNode*> &visitedNodes) {

    visitedNodes.insert(u);

    std::map<GraphNode*, edgeType> succs = u->getSuccessors();
    DEBUG(errs()<<"dfs ["<<u->getName()<<" "<<u->getLabel()<<"] succs size["<<succs.size()<<"]\n");

    for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
         succs.end(); succ != s_end; succ++) {
        if (visitedNodes.count(succ->first) == 0) {
            DEBUG(errs()<<"dfs ["<<u->getName()<<" "<<u->getLabel()<<"] succ["<<succ->first->getName()<<" "<<succ->first->getLabel()<<"]\n");
            dfsVisit(succ->first, visitedNodes);
        }
    }
    DEBUG(errs()<<"=====================  End succi dfs ["<<u->getName()<<" "<<u->getLabel()<<"]\n");

}

bool Graph::dfsVisit(GraphNode* u, GraphNode* stop,std::set<GraphNode*> &visitedNodes) {

    if(u==stop) {
        DEBUG(errs()<<"dfs - source=destination");
        return true;
    }
    visitedNodes.insert(u);

    std::map<GraphNode*, edgeType> succs = u->getSuccessors();

    for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
         succs.end(); succ != s_end; succ++) {
        if (visitedNodes.count(succ->first) == 0) {
            //DEBUG(errs()<<"dfs["<<succ->first->getName()<<" "<<succ->first->getLabel()<<"]\n");
            if(succ->first==stop) {
                //DEBUG(errs()<<"dfs-stop["<<succ->first->getName()<<" "<<succ->first->getLabel()<<"]\n");
                visitedNodes.insert(stop);
                return true;
            } else{
                //DEBUG(errs()<<"dfs["<<succ->first->getName()<<" "<<succ->first->getLabel()<<"]\n");
                if(dfsVisit(succ->first,stop, visitedNodes)) return true;
            }
        }
    }
    return false;

}

bool Graph::dfsVisitVec(GraphNode* u, GraphNode* stop,std::vector<GraphNode*> &visitedNodes) {

    if(u==stop) {
        DEBUG(errs()<<"dfs - source=destination");
        return true;
    }
    visitedNodes.push_back(u);

    std::map<GraphNode*, edgeType> succs = u->getSuccessors();

    for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end =
         succs.end(); succ != s_end; succ++) {
        if (std::count(visitedNodes.begin(),visitedNodes.end(),succ->first) == 0){
            //DEBUG(errs()<<"dfs["<<succ->first->getName()<<" "<<succ->first->getLabel()<<"]\n");
            if(succ->first==stop) {
               //DEBUG(errs()<<"dfs-stop["<<succ->first->getName()<<" "<<succ->first->getLabel()<<"]\n");
                visitedNodes.push_back(stop);
//    		visitedNodesMap[std::make_pair<GraphNode*, GraphNode*>(u, stop)] = visitedNodes;
    		visitedNodesMap[std::make_pair<GraphNode*, GraphNode*>(u, stop)] = true;
                return true;
            } else{
		//std::vector<GraphNode*> vstdNodes = visitedNodesMap[std::make_pair<GraphNode*, GraphNode*>(succ->first, stop)];
		bool vstdNodes = visitedNodesMap[std::make_pair<GraphNode*, GraphNode*>(succ->first, stop)];
		//if(vstdNodes.empty()){ 
		if(!vstdNodes){ 
                	//DEBUG(errs()<<"dfs["<<succ->first->getName()<<" "<<succ->first->getLabel()<<"]\n");
                	if(dfsVisitVec(succ->first,stop, visitedNodes)) {
    				//visitedNodesMap[std::make_pair<GraphNode*, GraphNode*>(u, stop)] = visitedNodes;
    				visitedNodesMap[std::make_pair<GraphNode*, GraphNode*>(u, stop)] = true;
				return true;
			} else {
                		DEBUG(errs()<<"dfsVisitVec(succ->frist,stop,visitedNodes = false["<<succ->first->getName()<<" "<<succ->first->getLabel()<<"]\n");
			}
		} else {
                	//DEBUG(errs()<<"Not dfs.  VstdNodes is not empty, size=["<<vstdNodes.size()<<"]\n");
                	DEBUG(errs()<<"Not dfs.  VstdNodes is not empty\n");
			return true;
		}
            }
        }
	DEBUG(errs()<<"=====================End for sucss: ["<<succ->first->getName()<<" "<<succ->first->getLabel()<<"]\n");
    }
    return false;

}

void Graph::dfsVisitBack(GraphNode* u, std::set<GraphNode*> &visitedNodes) {

    visitedNodes.insert(u);

    DEBUG(errs()<<"dfsVisitBack - getPredessor of["<<u->getName()<<"]\n");

    std::map<GraphNode*, edgeType> preds = u->getPredecessors();

    for (std::map<GraphNode*, edgeType>::iterator pred = preds.begin(), s_end =
         preds.end(); pred != s_end; pred++) {
        if (visitedNodes.count(pred->first) == 0) {
            DEBUG(errs()<<"dfsVisitBack - predecessors["<<pred->first->getName()<<" "<<pred->first->getLabel()<<"]\n");
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

void Graph::saveToFile(std::string fileName) {

    std::string ErrorInfo;
    raw_fd_ostream File(fileName.c_str(), ErrorInfo);
    if (!ErrorInfo.empty()) {
        errs() << "Error opening file " << fileName
               << " for writing! Error Info: " << ErrorInfo << " \n";
        return;
    }

    std::map<GraphNode*, int> DefinedNodes;
    for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
         != end; node++) {

        if (DefinedNodes.count(*node) == 0) {
            File    << "|N"
                    << "|" << (*node)->getClass_Id()
                    << "|" << (*node)->getId()
                    << "|" << (*node)->getName()
                    << "|" << (*node)->getShape()
                    << "|" << (*node)->getStyle()
                    << "|" << (*node)->getLabel()
                    << "|\n";
            DefinedNodes[*node] = 1;
        }

        std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();

        for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
             s_end = succs.end(); succ != s_end; succ++) {

            if (DefinedNodes.count(succ->first) == 0) {
                File << "|N|"
                     << (succ->first)->getClass_Id() << "|"
                     << (succ->first)->getId() << "|"
                     << (succ->first)->getName() << "|"
                     << (succ->first)->getShape() << "|"
                     << (succ->first)->getStyle() << "|"
                     << (succ->first)->getLabel() << "|\n";
                DefinedNodes[succ->first] = 1;
            }

            //Source
            File << "|E" << "|" << (*node)->getName() << "|";

            //Destination
            File << (succ->first)->getName() << "|\n";

            /*
                if (succ->second == etControl)
                    (*stream) << " [style=dashed]";
    */
        }
    }
}
/*
int Graph::loadFromFile(std::string fileName, Graph *depGraph) {
    std::string ErrorInfo;
    ifstream fin(fileName.c_str());
    if(!fin){
        DEBUG(errs() << "Error opening file " << fileName
              << " for load Graph! " << ErrorInfo << " \n";);
        return -1;
    }


    std::map<std::string, GraphNode*> nodesMap;

    //Graph GF(this->AS); // FIXME use the correct AS or create a constructor without parameters
    std::string value;
    while(!fin.eof()){
        getline(fin,value, '|');
        //check if it is a node (N) ou an edge (E)
        if(value=="N"){
            GraphNode* n;
            getline(fin,value,'|');// get the Node's Class
            int class_id = atoi(value.c_str());
            switch(class_id){
            case 1:
                n = new OpNode();
                break;
            case 2:
                n = new VarNode();
                break;
            case 3:
                n = new CallNode();
                break;
            case 4:
                n = new MemNode();
                break;
            case 5:
                n = new NetWriteNode();
                netWrite.insert(n);
                DEBUG(errs()<<"Load NetWriteNode\n");
                break;
            case 6:
                n = new NetReadNode();
                netRead.insert(n);
                DEBUG(errs()<<"Load NetReadNode\n");
                break;
            case 7:
            	n = new InputNode();
            	break;
            default:
                //                              n = new GraphNode();
                DEBUG(errs() << "Class_ID not known!");
                break;
            }

            getline(fin,value,'|');// get the Node's Id
            n->setId(atoi(value.c_str()));

            getline(fin,value,'|');// get the Node's Name
            n->setName(value);

            getline(fin,value,'|');// get the Node's Shape
            n->setShape(value);

            getline(fin,value,'|');// get the Node's Style
            n->setStyle(value);

            getline(fin,value,'|');// get the Node's Label
            n->setLabel(value);

            nodesMap[n->getName()] = n; //put the node in map to manage edge below
            depGraph->nodes.insert(n);

        }else if(value=="E"){
            //DEBUG(errs()<<"Load EDGE from file\n");
            //get source
            getline(fin,value,'|');
            GraphNode* source = nodesMap[value];
            //DEBUG(errs()<<"Source:["<<source->getName()<<"]\n");
            //get destination
            getline(fin,value,'|');
            //DEBUG(errs()<<"Dest value:["<<value<<"]\n");
            GraphNode* dest = nodesMap[value];
            //DEBUG(errs()<<"Dest:"<<dest->getName()<<"\n");
            //                  GF.addEdge(source,dest);
            source->connect(dest);
            getline(fin,value,'|');// get \n and ignore it
        }
    }
    return 0;
}
*/
void Graph::toDot(std::string s, raw_ostream *stream) {

    (*stream) << "digraph \"DFG for \'" << s << "\' function \"{\n";
    (*stream) << "label=\"DFG for \'" << s << "\' function\";\n";

    DEBUG(errs()<<"toDot start nodes size="<<nodes.size()<<"\n");
    std::map<GraphNode*, int> DefinedNodes;

    for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node
         != end; node++) {

        if (DefinedNodes.count(*node) == 0) {
            std::ostringstream stringStream;
            stringStream << (*node)->getName()
                         << "[shape="   << (*node)->getShape()
                         << " color="    << getNodeColor(*node)
                         << ",style="   << (*node)->getStyle()
                         << ",label=\""  << (*node)->getName() << " " << (*node)->getLabel()
                         << "\"]"
			<< "\n";
            //DEBUG(errs()<<stringStream.str());
            (*stream) << stringStream.str();
            DefinedNodes[*node] = 1;
        }

        std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();

        for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),
             s_end = succs.end(); succ != s_end; succ++) {

            //DEBUG(errs()<<"succs Iterator\n");
            //DEBUG(errs()<<"succs->first->getName()"<< (succ->first)->getName()<<"\n");
            if (DefinedNodes.count(succ->first) == 0) {
                std::ostringstream stringStream;
                stringStream << (succ->first)->getName()
                             << "[shape=" << (succ->first)->getShape()
                             << " color="    << getNodeColor(succ->first)
                             << ",style=" 	<< (succ->first)->getStyle()
                             << ",label=\"" << (succ->first)->getLabel()
                             << "\"]"
			     << "\n";
                DEBUG(errs()<<stringStream.str());
                (*stream) << stringStream.str();
                DefinedNodes[succ->first] = 1;
            }
            //Source
            std::ostringstream stringStream;
            stringStream << "\"" << (*node)->getName() << "\"";
            //DEBUG(errs()<<" source["<<stringStream.str()<<"]");
            stringStream << "->";

            //Destination
            stringStream << "\"" << (succ->first)->getName() << "\"";
            //DEBUG(errs()<<" dest["<<stringStream.str()<<"]");

            if (succ->second == etControl)
                stringStream << " [style=dashed]";

            stringStream << "\n";
            //DEBUG(errs()<<stringStream.str());
            (*stream) << stringStream.str();

        }

    }

    (*stream) << "}\n\n";

}

std::string Graph::getNodeColor(GraphNode *node) {

	return isNodeP1(node) ? "cyan" : "salmon";

}

bool Graph::isSendOrRecvNode(GraphNode *node)
{
	if((node->getLabel().find("Write") != std::string::npos) || (node->getLabel().find("Read") != std::string::npos))
		return true;

	return false;

}

bool Graph::isNodeP1(GraphNode *node)
{
	if (node->getName().find("P1") != std::string::npos)
		return true;

	return false;

}

void Graph::saveStatisticsToFile(std::string DaemonClient)
{
    //Save statistic file
    std::string statFileName ="NetDepGraph_statistics.txt";
    ifstream infile(statFileName.c_str());

    if(!infile.good()){
                ofstream File(statFileName.c_str(),std::ios_base::out | std::ios_base::app);
                File<<"Inst;\tNumLoads;\tNumStores;\tMemNodes;\tControlEdges;\tDataEdeges;\tNodes1;\tRecvNodes1;\tSendNodes1;\tSREdges;\tNodes2;\tRecvNodes2;\tSendNodes2;\tServ_Clien Name"<<endl;
                File.close();
    }
    ofstream File(statFileName.c_str(),std::ios_base::out | std::ios_base::app);

    
    string buffer;

    File << NumInsts << ";\t";
    File << NumLoads<< ";\t";
    File << NumStores<< ";\t";
    File << memNodes.size() << ";\t";
    File << getNumDataEdges() << ";\t";
    File << getNumControlEdges()<< ";\t";
    File << NumNodesP1.getValue() << ";\t";
    File << NumRecvNodesP1.getValue() << ";\t";
    File << NumSendNodesP1.getValue() << ";\t";
    File << NumSendRecvEdgesP1P2.getValue() << ";\t";
    File << NumNodesP2.getValue() << ";\t";
    File << NumRecvNodesP2.getValue() << ";\t";
    File << NumSendNodesP2.getValue() << ";\t";
    File << DaemonClient << endl;
    File.close();
}

void Graph::toDotFocusSendRecv(std::string s, raw_ostream *stream, string DaemonClient) {
    (*stream) << "digraph \"DFG for \'" << s << "\' function \"{\n";
    (*stream) << "label=\"DFG for \'" << s << "\' function\";\n";
    //DEBUG(errs()<<"toDot start\n");
    std::map<GraphNode*, int> NodesOfInterest;
    for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node != end; node++)
    {
			std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();
			for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),s_end = succs.end(); succ != s_end; succ++)
			{
				std::ostringstream stringStream;
				if((NodesOfInterest.count(*node) == 0) && (isSendOrRecvNode(*node) || isSendOrRecvNode(succ->first)))
				{
					NodesOfInterest[*node] = 1;
					stringStream << (*node)->getName() << "[shape="	 << (*node)->getShape() << " color=" << getNodeColor(*node);
					if(isSendOrRecvNode(*node)) stringStream << ",style=filled";
					else stringStream << ",style=" << (*node)->getStyle();
					stringStream << ",label=\"" << (*node)->getName() << " " << (*node)->getLabel() << "\"]\n";
					(*stream) << stringStream.str();
				}
				if((NodesOfInterest.count(succ->first) == 0) && (isSendOrRecvNode(*node) || isSendOrRecvNode(succ->first)))
				{
					NodesOfInterest[succ->first] = 1;
					stringStream << (succ->first)->getName() << "[shape=" << (succ->first)->getShape() << " color=" << getNodeColor(succ->first);
					if(isSendOrRecvNode(succ->first)) stringStream << ",style=filled";
					else stringStream << ",style=" << (*node)->getStyle();
					stringStream << ",label=\"" << succ->first->getName() << " " << succ->first->getLabel() << "\"]\n";
					(*stream) << stringStream.str();
				}
				if (isSendOrRecvNode(*node)	|| isSendOrRecvNode(succ->first))
				{
					//Source
					std::ostringstream stringStream;
					stringStream << "\"" << (*node)->getName() << "\"";
					stringStream << "->";
					//Destination
					stringStream << "\"" << (succ->first)->getName() << "\"";
//            		stringStream << "[color=yellow penwidth=3]";
					stringStream << "\n";
					(*stream) << stringStream.str();

				}

			}
    	}
    (*stream) << "}\n\n";

}




void Graph::toDotFocusSendRecvWithMemAndInputNodes(std::vector<std::vector<GraphNode*> > visitedNodesList, std::string s, raw_ostream *stream, string DaemonClient) {

    (*stream) << "digraph \"DFG for \'" << s << "\' function \"{\n";
    (*stream) << "label=\"DFG for \'" << s << "\' function\";\n";

    //DEBUG(errs()<<"toDot start\n");
    std::map<GraphNode*, int> NodesOfInterest;

    for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node != end; node++)
    {
			std::map<GraphNode*, edgeType> succs = (*node)->getSuccessors();

			for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(),s_end = succs.end(); succ != s_end; succ++)
			{

				std::ostringstream stringStream;

				if((NodesOfInterest.count(*node) == 0) && (isSendOrRecvNode(*node) || isSendOrRecvNode(succ->first)))
				{
					NodesOfInterest[*node] = 1;

					stringStream << (*node)->getName() << "[shape="	 << (*node)->getShape() << " color=" << getNodeColor(*node);

					if(isSendOrRecvNode(*node))
						stringStream << ",style=filled";
					else
						stringStream << ",style=" << (*node)->getStyle();

					stringStream << ",label=\"" << (*node)->getName() << " " << (*node)->getLabel() << "\"]\n";

					(*stream) << stringStream.str();

				}
				if((NodesOfInterest.count(succ->first) == 0) && (isSendOrRecvNode(*node) || isSendOrRecvNode(succ->first)))
				{
					NodesOfInterest[succ->first] = 1;

					stringStream << (succ->first)->getName() << "[shape=" << (succ->first)->getShape() << " color=" << getNodeColor(succ->first);

					if(isSendOrRecvNode(succ->first))
						stringStream << ",style=filled";
					else
						stringStream << ",style=" << (*node)->getStyle();

					stringStream << ",label=\"" << succ->first->getName() << " " << succ->first->getLabel() << "\"]\n";

					(*stream) << stringStream.str();

				}


				if (isSendOrRecvNode(*node)	|| isSendOrRecvNode(succ->first))
				{

					//Source
					std::ostringstream stringStream;
					stringStream << "\"" << (*node)->getName() << "\"";
					stringStream << "->";

					//Destination
					stringStream << "\"" << (succ->first)->getName() << "\"";
//            		stringStream << "[color=yellow penwidth=3]";
					stringStream << "\n";

					(*stream) << stringStream.str();

				}

			}
    	}

    std::vector<GraphNode*> visitedNodes;
    std::map<std::string,std::string> visitedNodesRelations;
    GraphNode *previous;
    std::string label,relation;

	for (int i = 0; i < visitedNodesList.size(); i++) {

				visitedNodes = visitedNodesList[i];

				for (std::vector<GraphNode*>::iterator node = visitedNodes.begin(), end =
						visitedNodes.end(); node != end; node++) {

					if(node == visitedNodes.begin())
						previous = *node;

					label = (*node)->getLabel();


					if(((*node)->getLabel().find("Memory") != std::string::npos) ||
							(label.find("Write") != std::string::npos) || (label.find("Read") != std::string::npos)
							|| (label.find("INPUT") != std::string::npos) || (node == visitedNodes.begin()))
					{
						if(NodesOfInterest.count((*node)) == 0)
						{
							(*stream) << "\"" << (*node)->getName() << "\"" << "[label=\"" << label + "\" color=\"" << getNodeColor(*node) << "\", style=filled]"; //label << program << "\"" << "[label=\"" << label + "\" color=\"" << color << "\", style=filled]";
							(*stream) << "\n";

							NodesOfInterest[*node] = 1;

						}
					}


					if(node != visitedNodes.begin())
					{
						if(((*node)->getLabel().find("Memory") != std::string::npos) ||
								(label.find("Write") != std::string::npos) || (label.find("Read") != std::string::npos)
								|| (label.find("INPUT") != std::string::npos))
						{
							relation = "\"" + previous->getName() + "\"";
							relation += "->";
							relation += "\"" + (*node)->getName() + "\"";
							relation += "\n";

							previous = *node;


							if (visitedNodesRelations.count(relation) == 0)
							{
								visitedNodesRelations[relation] =  relation;
								(*stream) << relation;
							}


						}
					}
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
GraphNode* Graph::addInst(Value *v, Function *f) {

    GraphNode *Op, *Var, *Operand;

    /////////////////////////////////////////////////////////////
    NetReadNode *netReadNode=NULL;
    NetWriteNode *netWriteNode=NULL;
    int programId=0;
    StringRef fname = f->getName().str();
    if(fname.startswith("ttt_")){
        programId = 2;
    } else {
        programId = 1;
    }

    CallInst* CI = dyn_cast<CallInst> (v);
    bool hasVarNode = true;

    if (isValidInst(v)) { //If is a data manipulator instruction
        Var = this->findNode(v,programId);
        if(Var!= NULL){
            if(Var->getProgramID() != programId){ // this test is necessary for constants in netdepgraph
                Var = NULL;
            }
        }

        /*
             * If Var is NULL, the value hasn't been processed yet, so we must process it
             *
             * However, if Var is a Pointer, maybe the memory node already exists but the
             * operation node aren't in the graph, yet. Thus we must process it.
             */
        if (Var == NULL || (Var != NULL && findOpNode(v,programId) == NULL)) { //If it has not processed yet

            //If Var isn't NULL, we won't create another node for it
            if (Var == NULL) {

                if (CI) {
                    hasVarNode = !CI->getType()->isVoidTy();
                }

                if (hasVarNode) {
                    if (StoreInst* SI = dyn_cast<StoreInst>(v)){
                        Var = addInst(SI->getOperand(1),f); // We do this here because we want to represent the store instructions as a flow of information of a data to a memory node
			Var->setNodeCodeLineAndName(SI);
                        DEBUG(errs() << "Store Node" << Var << " Count "<< Var->getCount() << "File " <<Var->getNodeCodeName() << "Line " <<Var->getNodeCodeLine() <<"\n");
		    }
                    else if ((!isa<Constant> (v)) && isMemoryPointer(v)) {
                        Var = new MemNode( USE_ALIAS_SETS ? AS->getValueSetKey(v) : 0, AS);
                        memNodes[USE_ALIAS_SETS ? AS->getValueSetKey(v) : 0] = Var;
			if (isa<Instruction> (v)) {
				Instruction *I =  dyn_cast<Instruction> (v); 
				Var->setNodeCodeLineAndName(I);
                        	DEBUG(errs() << "Memory Node" << Var << " Count "<< Var->getCount() << "File " <<Var->getNodeCodeName() << "Line " <<Var->getNodeCodeLine() <<"\n");
			} else {
                        	DEBUG(errs() << "Memory Node created but v is not an instruction" << Var <<"\n");
			}
                        if(programId == 1) {
				memNodes1.insert(Var);
			}
                        else {
				memNodes2.insert(Var);
			}
                    } else {
                        Var = new VarNode(v);
			if (isa<Instruction> (v)) {
				Instruction *I =  dyn_cast<Instruction> (v); 
				Var->setNodeCodeLineAndName(I);
                        	DEBUG(errs() << "Var Node" << Var << " Count "<< Var->getCount() << "File " <<Var->getNodeCodeName() << "Line " <<Var->getNodeCodeLine() <<"\n");
			} else {
                        	DEBUG(errs() << "Var Node created but v is not an instruction" << Var <<"\n");
			}
                        if(programId==1){
                            varNodes1[v] = Var;
			}
                        else {
			    varNodes2[v] = Var;
			}
                    }
		    if(isa<LoadInst> (v)){
			NumLoads++;	
			LoadInst* LI = dyn_cast<LoadInst>(v);
                        //DEBUG(errs() << "Load Inst " << *LI <<"\n");
			Value *m = LI->getPointerOperand();
			//Value *m = LI->getOperand(1);
			GraphNode *n = this->findNode(m,programId);	
			if(!n) {
                        	DEBUG(errs() << "Load - node not found["<<*m<<"\n");
				NumLoadsNotCount++;
                        	//Var = new MemNode( USE_ALIAS_SETS ? AS->getValueSetKey(m) : 0, AS);
                        	//memNodes[USE_ALIAS_SETS ? AS->getValueSetKey(v) : 0] = Var;
                        	//if(programId == 1) memNodes1.insert(Var);
                        	//else memNodes2.insert(Var);
				//n->add();
                        	//DEBUG(errs() << "Load - Node created - PointerOperand " << m << "GraphNode " << n << " Count "<< n->getCount() <<"\n");
			}else{
				n->add();
                        	DEBUG(errs() << "Load - PointerOperand " << m << "GraphNode " << n <<"\n");
			}
		    }
                    //DEBUG(errs()<<"Insert Var Node, name["<<Var->getName()<<"] program ID["<< programId << "]\n");
                    Var->setProgramID(programId);
                    nodesInsert(nodes, Var,programId);
                }

            }
            if (isa<Instruction> (v)) {
	       Instruction *I =  dyn_cast<Instruction> (v);
	       StringRef instName = I->getName().str();
               if(instName.equals("receive_callback")) {
                        DEBUG(errs() << "NetDepGraph: 1. Receive Instruction: "<< *I << "] Name:["<< instName << "]\n");
                        netReadNode = new NetReadNode(I);
			if(programId==1) NumRecvNodesP1++;
			else NumRecvNodesP2++;
                        netReadNode->setLabel(instName);
                        netReadNode->setNodeCodeLineAndName(I);
                        DEBUG(errs() << "Net Read Node" << netReadNode << " Count "<< netReadNode->getCount() << "File " <<netReadNode->getNodeCodeName() << "Line " <<netReadNode->getNodeCodeLine() <<"\n");
                        if (USE_ELEVATOR) {
                            set<int> levels = EL->getNodeLevels(static_cast<ProgramNumber>(programId),RECV,netReadNode->getNodeCodeName(),netReadNode->getNodeCodeLine());
                            netReadNode->setNodeLevels(levels);
                        }
                        DEBUG(errs()<<"teste_rrr: "<< netReadNode->getLabel()<<"\n");
                        netReadNode->setProgramID(programId);
                    	nodesInsert(nodes, Var,programId);
                        netRead.insert(netReadNode);
                        DEBUG(errs()<<"Insert NetReadNode, name["<<netReadNode->getName()<<"] label ["<<  netReadNode->getLabel() <<"] program ID["<< netReadNode->getProgramID() << "]\n");
                        for (std::set<GraphNode*>::iterator node = netWrite2.begin(), end = netWrite2.end(); node != end; node++){
                                if((*node)->hasLevelIntersection(netReadNode->getNodeLevels())){
                                        (*node)->connect(netReadNode);
					NumSendRecvEdgesP1P2++;
                                        DEBUG(errs()<<"Connect new NetReadNode with known NetWriteNodes: " << (*node)->getLabel()<<"->"<<netReadNode->getLabel() <<"\n");
                                }

                        }
/*
                        User *u = dyn_cast<User>(I);
                        //DEBUG(errs() << "User in process_post[" << *u << "] with operands[" << u->getNumOperands()<<"] \n");
                        unsigned int numOperands = u->getNumOperands();
                        DEBUG(errs() << "2. NetDepGraph: User in receive_callback[" << *u << "] with operands[" << numOperands<<"] \n");
                        //for(int i = 0; i < numOperands; i++){
                        for(int i = 0; i < 1; i++){
                                Value *V = u->getOperand(i);
                                DEBUG(errs() << "\n-----\n3.NetDepGraph: User in receive_callback operand[" << i <<"] operand ["<< *V <<"]\n");
                                if (V->getType()->isPointerTy()){
                                        if(programID == 2){
                                                DEBUG(errs() << "4. NetDepGraph: ttt Insert the Operand of receive_callback in NetDepGraph ["<< *V<< "]\n");
                                                Input *i = new Input(V, 1, 2);
                                                i->setInstName("receive_callback");
                                                inputDepValues2WithNetSources.insert(i);
                                                netInputDepValues2.insert(i);
                                        }else{
                                                DEBUG(errs() << "4. NetDepGraph: Insert the Operand of receive_callback in NetDepGraph["<< *V<< "]\n");
                                                Input *i = new Input(V, 1, 1);
                                                i->setInstName("receive_callback");
                                                inputDepValuesWithNetSources.insert(i);
                                                netInputDepValues.insert(i);
                                        }
                                }
                        }

                } 
*/
	       }
                //DEBUG(errs()<< v->getName()<< " is an Instruction\n" ;);
                if (CI) {
                    Op = new CallNode(CI);
                    Op->setProgramID(programId);
		    Op->setNodeCodeLineAndName(CI);
		    nodesInsert(nodes, Op, programId);
                    //DEBUG(errs()<<"New CallNode, name["<<Op->getName()<<"] program ID["<< programId << "]\n");
                    callNodes[CI] = Op;
                    /////////////////////////////////////////////////////////////////////
                    Function* f = CI->getCalledFunction();
                    if(f!=NULL){
                        std::string calledFunction =  f->getName().str();
                        DEBUG(errs()<<"Called Function, name["<<calledFunction<<"] program ID["<< programId << "]\n");
                        std::string DebugInfo;
                        raw_fd_ostream File("debugArq1", DebugInfo,2);
                        File << calledFunction << "\n";
                        //if(NetNode == NULL and ("read" == calledFunction or "write" == calledFunction)){
			std::string recv("rrr_");
			std::string trecv("ttt_rrr_");
			std::string send("sss_");
			std::string tsend("ttt_sss_");

                        if(calledFunction.compare(0,recv.length(),recv)==0){
                            netReadNode = new NetReadNode(CI);
 			    if(programId==1) NumRecvNodesP1++;
                            else NumRecvNodesP2++;
                            netReadNode->setLabel(calledFunction);
                            netReadNode->setNodeCodeLineAndName(CI);
                            DEBUG(errs() << "Net Read Node" << netReadNode << " Count "<< netReadNode->getCount() << "File " <<netReadNode->getNodeCodeName() << "Line " <<netReadNode->getNodeCodeLine() <<"\n");
                            if (USE_ELEVATOR) {
                                set<int> levels = EL->getNodeLevels(static_cast<ProgramNumber>(programId),RECV,netReadNode->getNodeCodeName(),netReadNode->getNodeCodeLine());
                                netReadNode->setNodeLevels(levels);
                            }
                            DEBUG(errs()<<"teste_rrr: "<< netReadNode->getLabel()<<"\n");
                            netReadNode->setProgramID(programId);
		    	    nodesInsert(nodes, netReadNode, programId);
                            netRead.insert(netReadNode);
                            DEBUG(errs()<<"Insert NetReadNode, name["<<netReadNode->getName()<<"] label ["<<  netReadNode->getLabel() <<"] program ID["<< netReadNode->getProgramID() << "]\n");
                            for (std::set<GraphNode*>::iterator node = netWrite2.begin(), end = netWrite2.end(); node != end; node++){
                            	if((*node)->hasLevelIntersection(netReadNode->getNodeLevels())){
                                	(*node)->connect(netReadNode);
					NumSendRecvEdgesP1P2++;
                                	DEBUG(errs()<<"Connect new NetReadNode with known NetWriteNodes: " << (*node)->getLabel()<<"->"<<netReadNode->getLabel() <<"\n");
                            	}

                            }
                        }else if(calledFunction.compare(0,trecv.length(),trecv)==0){
                            netReadNode = new NetReadNode(CI);
 			    if(programId==1) NumRecvNodesP1++;
                            else NumRecvNodesP2++;
                            netReadNode->setLabel(calledFunction);
                            netReadNode->setNodeCodeLineAndName(CI);
                            DEBUG(errs() << "Net Read Node" << netReadNode << " Count "<< netReadNode->getCount() << "File " <<netReadNode->getNodeCodeName() << "Line " <<netReadNode->getNodeCodeLine() <<"\n");
                            if (USE_ELEVATOR) {
                                set<int> levels = EL->getNodeLevels(static_cast<ProgramNumber>(programId),RECV,netReadNode->getNodeCodeName(),netReadNode->getNodeCodeLine());
                                netReadNode->setNodeLevels(levels);
                            }
                            DEBUG(errs()<<"teste_ttt_rrr: "<< netReadNode->getLabel()<<"\n");
                            netReadNode->setProgramID(programId);
		    	    nodesInsert(nodes, netReadNode, programId);
                            DEBUG(errs()<<"Insert NetReadNode, name["<<netReadNode->getName()<<"] label ["<<  netReadNode->getLabel() <<"] program ID["<< netReadNode->getProgramID() << "]\n");
                            netRead2.insert(netReadNode);
                            for (std::set<GraphNode*>::iterator node = netWrite.begin(), end = netWrite.end(); node != end; node++){
                            	if((*node)->hasLevelIntersection(netReadNode->getNodeLevels())){
                            		(*node)->connect(netReadNode);
					NumSendRecvEdgesP1P2++;
                                	DEBUG(errs()<<"Connect new NetReadNode with known NetWriteNodes: " << (*node)->getLabel()<<"->"<<netReadNode->getLabel() <<"\n");
                            	}
                            }
                        } else if(calledFunction.compare(0,send.length(),send)==0){
                            netWriteNode = new NetWriteNode(CI);
 			    if(programId==1) NumSendNodesP1++;
                            else NumSendNodesP2++;
                            netWriteNode->setLabel(calledFunction);
                            netWriteNode->setNodeCodeLineAndName(CI);
                            DEBUG(errs() << "Net Write Node" << netWriteNode << " Count "<< netWriteNode->getCount() << "File " <<netWriteNode->getNodeCodeName() << "Line " <<netWriteNode->getNodeCodeLine() <<"\n");
                            if (USE_ELEVATOR) {
                                set<int> levels = EL->getNodeLevels(static_cast<ProgramNumber>(programId),SEND,netWriteNode->getNodeCodeName(),netWriteNode->getNodeCodeLine());
                                netWriteNode->setNodeLevels(levels);
                            }
                            DEBUG(errs()<<"teste_sss: "<< netWriteNode->getLabel()<<"\n");
                            netWriteNode->setProgramID(programId);
                            netWrite.insert(netWriteNode);
		    	    nodesInsert(nodes, netWriteNode, programId);
                            DEBUG(errs()<<"Insert NetWriteNode, name["<<netWriteNode->getName()<<"] label ["<< netWriteNode->getLabel() << "] program ID["<< netWriteNode->getProgramID() << "]\n");
                            for (std::set<GraphNode*>::iterator node = netRead2.begin(), end = netRead2.end(); node != end; node++){

                            	if((*node)->hasLevelIntersection(netWriteNode->getNodeLevels())){

                                netWriteNode->connect(*node);
				NumSendRecvEdgesP1P2++;
                                DEBUG(errs()<<"Connect new NetWriteNode with known NetReadNodes: "
                                                                                                                    <<netWriteNode->getLabel()<< "->"<<(*node)->getLabel()
                                      <<"\n");
                            	}
                            }
                        } else if(calledFunction.compare(0,tsend.length(),tsend)==0){
                            netWriteNode = new NetWriteNode(CI);
 			    if(programId==1) NumSendNodesP1++;
                            else NumSendNodesP2++;
                            netWriteNode->setLabel(calledFunction);
                            netWriteNode->setNodeCodeLineAndName(CI);
                            DEBUG(errs() << "Net Write Node" << netWriteNode << " Count "<< netWriteNode->getCount() << "File " <<netWriteNode->getNodeCodeName() << "Line " <<netWriteNode->getNodeCodeLine() <<"\n");
                            if (USE_ELEVATOR) {
                                set<int> levels = EL->getNodeLevels(static_cast<ProgramNumber>(programId),SEND,netWriteNode->getNodeCodeName(),netWriteNode->getNodeCodeLine());
                                netWriteNode->setNodeLevels(levels);
                            }
                            netWriteNode->setProgramID(programId);
                            DEBUG(errs()<<"teste_ttt_sss: "<< netWriteNode->getLabel()<<"\n");
		    	    nodesInsert(nodes, netWriteNode, programId);
                            DEBUG(errs()<<"Insert NetWriteNode, name["<<netWriteNode->getName()<<"] label ["<< netWriteNode->getLabel() << "] program ID["<< netWriteNode->getProgramID() << "]\n");
                            netWrite2.insert(netWriteNode);
                            for (std::set<GraphNode*>::iterator node = netRead.begin(), end = netRead.end(); node != end; node++){

                            	if((*node)->hasLevelIntersection(netWriteNode->getNodeLevels())){

                                netWriteNode->connect(*node);
				NumSendRecvEdgesP1P2++;
                                DEBUG(errs()<<"Connect new NetWriteNode with known NetReadNodes: "
                                                                                                                    <<netWriteNode->getLabel()<< "->"<<(*node)->getLabel()
                                      <<"\n");
                            	}
                            }

                        }else{
                            DEBUG(errs()<<"Called function ["<<calledFunction<<"] is not a network function\n");
			}

                    }else{

                        //DEBUG(errs()<<"Indirect function invocation\n");
                    }
                    /////////////////////////////////////////////////////////////////////

                } else {
		    Instruction *I = dyn_cast<Instruction> (v);
                    Op = new OpNode(I->getOpcode(), v);
                    Op->setProgramID(programId);
		    Op->setNodeCodeLineAndName(I);
                }
                opNodes[v] = Op;
                Op->setProgramID(programId);
                DEBUG(errs()<<"new OpNode["<<Op->getName()<<"] Label: ["<<Op->getLabel()<<"]\n");
		nodesInsert(nodes, Op, programId);
                if (hasVarNode)
                    Op->connect(Var);
                //Connect the operands to the OpNode
                for (unsigned int i = 0; i < cast<User> (v)->getNumOperands(); i++) {
		    if(isa<StoreInst> (v)){
			StoreInst* SI = dyn_cast<StoreInst>(v);
			Value *m = SI->getPointerOperand();
			GraphNode *n = this->findNode(m,programId);	
			n->add();
                        //DEBUG(errs() << "Store - PointerOperand " << m << "GraphNode " << n << " Count "<< n->getCount() <<"\n");
		    }
                    if (isa<StoreInst> (v) && i == 1)
                        continue; // We do this here because we want to represent the store instructions as a flow of information of a data to a memory node

                    Value *v1 = cast<User> (v)->getOperand(i);
                    Operand = this->addInst(v1,f);

                    if (Operand != NULL)
                        Operand->connect(Op);
                    /////////////////////////////////////////////////////////////////////
                    if(netReadNode != NULL) {
                        if(Operand->getClass_Id()==4){ //Link the MenNode Operant with NetReadNode
                            netReadNode->connect(Operand);
                            DEBUG(errs() << "NetReadDone Operand["<<Operand->getLabel()<<"]\n");
                        }
                    }
                    if(netWriteNode != NULL) { //Link the MenNode Operand with NetWriteNode
                        if(Operand->getClass_Id()==4){
                            Operand->connect(netWriteNode);
                            DEBUG(errs() << "NetWriteDone Operand\n");
                        }
                    }
                    /////////////////////////////////////////////////////////////////////

                }
            } else {
                //DEBUG(errs()<< v->getName()<< " is not an Instruction\n";);
            }
        }

        return Var;
    } else {
        //DEBUG(errs()<< v->getName()<< " is not a valid Instruction\n";);
    }
    return NULL;
}

void Graph::addEdge(int programId, GraphNode* src, GraphNode* dst, edgeType type) {

    nodesInsert(nodes, src, programId);
    nodesInsert(nodes, dst, programId);
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
GraphNode* Graph::findNode(Value *op, int programId) {

    if ((!isa<Constant> (op)) && isMemoryPointer(op)) {
        int index = USE_ALIAS_SETS ? AS->getValueSetKey(op) : 0;
        if (memNodes.count(index)){
            //DEBUG(errs()<<"findNode memNodes["<<index<<"]=" <<(memNodes[index]->getLabel()) <<"\n");
            return memNodes[index];
        }
    } else {
        if(programId == 1){
            if (varNodes1.count(op))
                return varNodes1[op];
        } else{
            if (varNodes2.count(op))
                return varNodes2[op];
        }
    }

    return NULL;
}

GraphNode* Graph::findNetNodeByCodeLineAndName(unsigned codeLine, std::string codeName, int Program){

	if(Program == 1)
	{
		for(std::set<GraphNode*>::iterator node = netRead.begin(), GNE = netRead.end();
			node != GNE; node++)
		{
			if(((*node)->getNodeCodeLine() == codeLine) && ((*node)->getNodeCodeName() == codeName))
			{
				return *node;
			}
		}
		for(std::set<GraphNode*>::iterator node = netWrite.begin(), GNE = netWrite.end();
			node != GNE; node++)
		{
			if(((*node)->getNodeCodeLine() == codeLine) && ((*node)->getNodeCodeName() == codeName))
			{
				return *node;
			}
		}
	}
	else
	{
		for(std::set<GraphNode*>::iterator node = netRead.begin(), GNE = netRead.end();
			node != GNE; node++)
		{
			if(((*node)->getNodeCodeLine() == codeLine) && ((*node)->getNodeCodeName() == codeName))
			{
				return *node;
			}
		}
		for(std::set<GraphNode*>::iterator node = netWrite.begin(), GNE = netWrite.end();
			node != GNE; node++)
		{
			if(((*node)->getNodeCodeLine() == codeLine) && ((*node)->getNodeCodeName() == codeName))
			{
				return *node;
			}
		}

	}

    return NULL;
}

std::set<GraphNode*> Graph::findNodes(std::set<Value*> values,int programID) {

    std::set<GraphNode*> result;

    for (std::set<Value*>::iterator i = values.begin(), end = values.end(); i
         != end; i++) {

        if (GraphNode* node = findNode(*i,programID)) {
            result.insert(node);
        }

    }

    return result;
}

OpNode* llvm::Graph::findOpNode(llvm::Value* op, int programID) {

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
		if(nodes.count(node)){
                	nodes.erase(node); 
			if (node->getProgramID()==1) NumNodesP1--;
			else if (node->getProgramID()==2) NumNodesP2--;
			else DEBUG(errs()<<"Program ID not set " << node->getName() <<"] label:["<<node->getLabel()<<"]\n");
		}
                delete node;
            }
            callNodes.erase(caller);
        }

    }

}

std::pair<GraphNode*, int> llvm::Graph::getNearestDependency(llvm::Value* sink,int sinkProgramID,
                                                             std::set<llvm::Value*> sources, int sourcesProgramID, bool skipMemoryNodes) {

    std::pair<llvm::GraphNode*, int> result;
    result.first = NULL;
    result.second = -1;

    if (GraphNode* startNode = findNode(sink,sinkProgramID)) {

        std::set<GraphNode*> sourceNodes = findNodes(sources,sourcesProgramID);

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
        llvm::Value* sink, int sinkProgramID, std::set<llvm::Value*> sources, int sourcesProgramId, bool skipMemoryNodes, bool skipNetworkNodes, bool *netdep) {
    for (std::set<Value *>::iterator Vit = sources.begin(), Vend =
         sources.end(); Vit != Vend; Vit++) {
        DEBUG(errs()<<"sources["<< (*Vit)->getName().str() <<"] ");
    }
    DEBUG(errs()<<"\n");

    std::map<llvm::GraphNode*, std::vector<GraphNode*> > result;
    DenseMap<GraphNode*, GraphNode*> parent;
    std::vector<GraphNode*> path;
    //bool netdeps=false;

    DEBUG(errs() << "--- Get every dep --- \n");
    if (GraphNode* startNode = findNode(sink,sinkProgramID)) {
        DEBUG(errs() << "found sink["<<sink->getName()<<"]\n");
        DEBUG(errs() << "Starting search from " << startNode->getName() <<"] label:["<<startNode->getLabel()<<"]\n");

        std::set<GraphNode*> sourceNodes = findNodes(sources,sourcesProgramId);
        for (std::set<GraphNode *>::iterator Nit = sourceNodes.begin(), Nend = sourceNodes.end(); Nit != Nend; Nit++) {
            DEBUG(errs()<<"sources["<< (*Nit)->getName() <<"] label:["<<(*Nit)->getLabel()<<"]");
        }
        DEBUG(errs()<<"\n");

        std::map<GraphNode*, int> nodeColor;
        std::list<GraphNode*> workList;
        int size = 0; //only to debug
        for (std::set<GraphNode*>::iterator Nit = nodes.begin(), Nend =
             nodes.end(); Nit != Nend; Nit++) {
            size++; //only to debug
            if (skipMemoryNodes && isa<MemNode> (*Nit))
                nodeColor[*Nit] = 1;
            else if(skipNetworkNodes){
                if((*Nit)->getClass_Id()==6 && (*Nit)->getProgramID()==startNode->getProgramID()){//get only netread of my program
                    nodeColor[*Nit] = 2;
                } else {//Reach a NetRead from other program
                    nodeColor[*Nit] = 1;
                }
            }
            else
                nodeColor[*Nit] = 0;

        }

        DEBUG(errs() << "Size nodeColor["<<size<<"]\n");
        workList.push_back(startNode);
        nodeColor[startNode] = 1;
        /*
                 * we will do a breadth search on the predecessors of each node,
                 * until we find one of the sources. If we don't find any, then the
                 * sink doesn't depend on any source.
                 */
        //              int pb = 1;
        //DEBUG(errs() << "sourceNodes.size()["<<sourceNodes.size()<<"] workList.size()["<<workList.size()<<"]\n");
        while (!workList.empty()) {
            GraphNode* workNode = workList.front();
            workList.pop_front();
            /*                	for (std::set<GraphNode*>::iterator Nit = sourceNodes.begin(), Nend =
                                sourceNodes.end(); Nit != Nend; Nit++) {
                DEBUG(errs()<<"sourceName["<< (*Nit)->getName() <<"] label["<<(*Nit)->getLabel()<<"] ");
            }
*/			//DEBUG(errs()<<"\n");
            if (sourceNodes.count(workNode)) {
                //Retrieve path
                path.clear();
                GraphNode* n = workNode;
                DEBUG(errs() << "Sink=[" << startNode->getName() << "]WorkNode["<< workNode->getName() <<"]\n");
                path.push_back(n);
                while (parent.count(n)) {
                    path.push_back(parent[n]);
                    n = parent[n];
                    //					DEBUG(errs() << "Sink=[" << startNode->getName() << "]WorkNode (n=parent[n]) ["<< n->getName() <<"]\n");
                }
                std::reverse(path.begin(), path.end());
                errs() << "Path: ";
                for (std::vector<GraphNode*>::iterator i = path.begin(), e = path.end(); i != e; ++i) {
                    errs() << (*i)->getLabel() <<" Prog. ID[" << (*i)->getProgramID() <<"] | ";
                }
                errs() << "\n";
                result[workNode] = path;
                DEBUG(errs() << "path.size()["<<path.size()<<"]\n");
            }
            std::map<GraphNode*, edgeType> preds = workNode->getPredecessors();
            //DEBUG(errs() << "workNode->Predecessors["<<preds.size()<<"] workList.size()["<<workList.size()<<"]");
            for (std::map<GraphNode*, edgeType>::iterator pred = preds.begin(),
                 pend = preds.end(); pred != pend; pred++) {
                if (nodeColor[pred->first] == 0) { // the node hasn't been processed yet
                    nodeColor[pred->first] = 1;
                    workList.push_back(pred->first);
                    //                                      pb++;
                    parent[pred->first] = workNode;
                } else if(skipNetworkNodes && nodeColor[pred->first] == 2){
                    *netdep=true;
                    DEBUG(errs() << "Reach a NetRead Node: StarNodeName[" << startNode->getName() <<"] ProgID ["<<startNode->getProgramID()<<
                                                 "pred->first->getName()["<<pred->first->getName()<<"] ProgID ["<<pred->first->getProgramID()<< "]\n");
                    nodeColor[pred->first] = 1;
                    workList.push_back(pred->first);
                    //                                      pb++;
                    parent[pred->first] = workNode;
                }

                /*                                if (nodeColor[pred->first] == 2) { // Reach a NetReadNode
                    DEBUG(errs() << "Reach a NetRead Node: pred->first->getName()["<<pred->first->getName()<<"]\n");
                                        workList.push_back(pred->first);
                                        //                                      pb++;
                                        parent[pred->first] = workNode;
                }
*/
            }
            //                      errs() << pb << "/" << size << "\n";
        }
    }
    //if(netdeps)NetDeps++;
    DEBUG(errs() << "result["<<result.size()<<"]\n");
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
    return varNodes1.size() + varNodes2.size();
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
    //	AU.addRequired<InputValues> ();
    //        AU.addRequired<InputDep> ();

    if (USE_ELEVATOR) {
        AU.addRequired< NetLevel<Function*> > ();
    }

    AU.setPreservesAll();
}

bool functionDepGraph::runOnFunction(Function &F) {

    AliasSets* AS = NULL;

    if (USE_ALIAS_SETS)
        AS = &(getAnalysis<AliasSets> ());

    NetLevel<Function*>* EL = NULL;
    if (USE_ELEVATOR) {
        EL = &(getAnalysis< NetLevel<Function*> > ());
    }

    //Making dependency graph
    depGraph = new llvm::Graph(AS, EL);
    Function *fp = &F;
    //Insert instructions in the graph
    for (Function::iterator BBit = F.begin(), BBend = F.end(); BBit != BBend; ++BBit) {
        for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit
             != Iend; ++Iit) {
            depGraph->addInst(Iit,fp);
        }
    }

    //We don't modify anything, so we must return false
    return false;
}

char functionDepGraph::ID = 0;
static RegisterPass<functionDepGraph> X("functionDepGraph",
                                        "Function Dependence Graph");

//Class netDepGraph
void netDepGraph::getAnalysisUsage(AnalysisUsage &AU) const {
    if (USE_ALIAS_SETS)
        AU.addRequired<AliasSets> ();

    if (USE_ELEVATOR) {
        AU.addRequired< NetLevel<Function*> > ();
    }

    AU.setPreservesAll();
}

bool netDepGraph::runOnModule(Module &M) {

    //	std::set<Value*> arrays;
    //	std::set<Value*> tarrays;
    //	std::set<Value*> funcHasArrays;
    AliasSets* AS = NULL;

    if (USE_ALIAS_SETS)
        AS = &(getAnalysis<AliasSets> ());

    NetLevel<Function*>* EL = NULL;
    if (USE_ELEVATOR) {
        EL = &(getAnalysis< NetLevel<Function*> > ());

        map< PairKey, set<int> > *program1SendNodes = EL->getNodesData(ONE, SEND);
        map< PairKey, set<int> > *program1RecvNodes = EL->getNodesData(ONE, RECV);
        map< PairKey, set<int> > *program2SendNodes = EL->getNodesData(TWO, SEND);
        map< PairKey, set<int> > *program2RecvNodes = EL->getNodesData(TWO, RECV);

        cerr << "program1SendNodes:" << program1SendNodes->size() << endl;
        cerr << "program1RecvNodes:" << program1RecvNodes->size() << endl;
        cerr << "program2SendNodes:" << program2SendNodes->size() << endl;
        cerr << "program2RecvNodes:" << program2RecvNodes->size() << endl;
    }

    //Get InputDeps
    //	InputDep &IV = getAnalysis<InputDep> ();
    // 	std::set<Value*> inputDepValues = IV.getInputDepValues();
    //      std::set<Value*> inputDepValues2 = IV.getInputDepValues(2);
    // 	DEBUG(errs() << "inputDepValues size=" << inputDepValues.size() << " inputDepValues2 size="<< inputDepValues2.size() << "\n");

    //Making dependency graph
    depGraph = new Graph(AS, EL);
    //////////////////////////////////////////////////
    string fileName = "saved.txt";
    // Restore data



    ///////////////////////////////////////////////////
    //	DenseMap<Function*, bool> funcHasArray;
    //	bool tmain = false;
    //Insert instructions in the graph

    for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {
        StringRef Name = Fit->getName().str();
        int programID;
        if(Name.startswith("ttt_")) programID = 2;
        else programID = 1;
        if(Name.startswith("ttt_rrr_")||Name.startswith("rrr_")){
            DEBUG(errs() << "Net Read Function: "<< Name << "\n");
            NetReadNode* netReadNode = new NetReadNode();
            netReadNode->setLabel(Name);
            netReadNode->setProgramID(programID);
            depGraph->getNodes().insert(netReadNode);
            if(Name.startswith("ttt_rrr_"))
            	depGraph->getNetReadNodes(2).insert(netReadNode);
            else if(Name.startswith("rrr_"))
            	depGraph->getNetReadNodes(1).insert(netReadNode);
            DEBUG(errs()<<"Insert NetReadNode, name["<<netReadNode->getName()<<"] label ["<<  netReadNode->getLabel() <<"] program ID["<< netReadNode->getProgramID() << "]\n");
        }

        Function *f = Fit;
        for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit
             != BBend; ++BBit) {
            			NumFunc++;
/*            std::string prefix = "ttt";
                    std::string name = Fit->getName();
            if(name.substr(0,prefix.size())== prefix){
                        DEBUG(errs()<<"Program 2 functions, name: "<<name << "\n");
                        tmain = true;
                    }else{
                        tmain = false;
                    }
*/

            for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit
                 != Iend; ++Iit) {
                if(Name.startswith("rrr_") or Name.startswith("sss_")){
                    DEBUG(errs() << "Function: "<< Name << "Instruction "<< *Iit << "\n");
                }
                depGraph->addInst(Iit,f);


                		NumInsts++;
               if (isa<StoreInst>(Iit)) NumStores++;
/*                        else if (AllocaInst* AI = dyn_cast<AllocaInst>(Iit)) {
                           Type* Ty = AI->getAllocatedType();
                   if (Ty->isArrayTy()) {
                    if(tmain) tarrays.insert(AI);
                    else      arrays.insert(AI);
                    NumArr++;
                    if (!funcHasArray[Fit]) {
                        funcHasArray[Fit] = true;
                        NumFuncArr++;
                    }
                   }
                             }
*/
            }
        }
    }
    //	DEBUG(errs()<<"Num Arrays="<<NumArr<<"NumFuncArrays"<<NumFuncArr;);

    //Connect formal and actual parameters and return values
    for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit) {
        // If the function is empty, do not do anything
        // Empty functions include externally linked ones (i.e. abort, printf, scanf, ...)
        if (Fit->begin() == Fit->end())
            continue;

        matchParametersAndReturnValues(*Fit);

    }
    ////////////////////////////////////////////////////////////
    //DEBUG(errs()<<"Restored DepGraph matchParameterAndReturnValues done\n");
//    std::string ErrorInfo;
//    raw_fd_ostream File("netdepgraph.dot",ErrorInfo);
//    depGraph->toDot("main",&File);
//    DEBUG(errs()<<"DepGraph toDot done\n");
//    // Create an output archive
//    depGraph->saveToFile("netsaved.txt");
//    DEBUG(errs()<<"DepGraph saveToFile done\n");
    ////////////////////////////////////////////////////////////

    //saveStatisticsToFile();
    //We don't modify anything, so we must return false
    return false;
}


void netDepGraph::matchParametersAndReturnValues(Function &F) {

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
    Function *f = &F;
    StringRef name = f->getName().str();
    int programID;
    if(name.startswith("ttt_")) programID = 2;
    else programID = 1;
    //Create the PHI nodes for the formal parameters
    for (i = 0, argptr = F.arg_begin(), e = F.arg_end(); argptr != e; ++i, ++argptr) {

        OpNode* argPHI = new OpNode(Instruction::PHI);
        argPHI->setProgramID(programID);
        GraphNode* argNode = NULL;
        argNode = depGraph->addInst(argptr,f);

        if (argNode != NULL)
            depGraph->addEdge(programID, argPHI, argNode);

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
            Parameters[i].second = depGraph->addInst(*AI,f);
        }

        // Match formal and real parameters
        for (i = 0; i < Parameters.size(); ++i) {

            depGraph->addEdge(programID,Parameters[i].second, Parameters[i].first);
        }

        // Match return values
        if (!noReturn) {

            OpNode* retPHI = new OpNode(Instruction::PHI);
            retPHI->setProgramID(programID);
            GraphNode* callerNode = depGraph->addInst(caller,f);
            depGraph->addEdge(programID,retPHI, callerNode);

            for (SmallPtrSetIterator<llvm::Value*> ri = ReturnValues.begin(),
                 re = ReturnValues.end(); ri != re; ++ri) {
                GraphNode* retNode = depGraph->addInst(*ri,f);
                depGraph->addEdge(programID,retNode, retPHI);
            }

        }

        // Real parameters are cleaned before moving to the next use (for safety's sake)
        for (i = 0; i < Parameters.size(); ++i)
            Parameters[i].second = NULL;
    }

    depGraph->deleteCallNodes(&F);
}

void llvm::netDepGraph::deleteCallNodes(Function* F) {
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

char netDepGraph::ID = 0;
static RegisterPass<netDepGraph> Y("netDepGraph",
                                   "Module Network Dependence Graph");

char ViewModuleDepGraph::ID = 0;
static RegisterPass<ViewModuleDepGraph> Z("view-netdepgraph",
                                          "View Module Dependence Graph");

