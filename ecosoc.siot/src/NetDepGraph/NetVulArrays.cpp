#include "NetVulArrays.h"
//#define DEBUG_TYPE "vulArrays"


static cl::opt<std::string> BC("BC", cl::desc("Specify Byte code name"), cl::value_desc("bytecode"));
static cl::opt<std::string> GRAPH("GRAPH", cl::desc("Specify graph mode"), cl::value_desc("graph"));


STATISTIC(arraysP1, "Program #1: The number of arrays");
STATISTIC(arraysP2, "Program #2: The number of arrays");
STATISTIC(arraysAbcP1, "Program #1: The number of ABCs total");
STATISTIC(arraysAbcP2, "Program #2: The number of ABCs total");
STATISTIC(vul1, "Program #1: The number of tainted data flows in program 1");
STATISTIC(vul2, "Program #2: The number of tainted data flows in program 2");
STATISTIC(abc1, "Program #1: The number of array bound checks points in program 1");
STATISTIC(abc2, "Program #2: The number of array bound checks points in program 2");
STATISTIC(crossVul1, "Program #1: The number of vul arrays in program 1 that depends of sources in program 2");
STATISTIC(crossVul2, "Program #2: The number of vul arrays in program 2 that depends of sources in program 1");
STATISTIC(crossABC1, "Program #1: The number of array bound checks points in program 1 that depends of sources in program 2");
STATISTIC(crossABC2, "Program #2: The number of array bound checks points in program 2 that depends of sources in program 1");
STATISTIC(netVul1, "Program #1: Net Read Arrays");
STATISTIC(netVul2, "Program #2: Net Read Arrays");
STATISTIC(netABC1, "Program #1: Net Read ABCs");
STATISTIC(netABC2, "Program #2: Net Read ABCs");
STATISTIC(fp1, "Program #1: False Positives (depends of the net but not depends of other input)");
STATISTIC(fp2, "Program #2: False Positives (depends of the net but not depends of other input)");


NetVulArrays::NetVulArrays() :
    ModulePass(ID) {    
    arraysP1=0;
    arraysP2=0;
    arraysAbcP1=0;
    arraysAbcP2=0;
    vul1=0;
    vul2=0;
    abc1=0;
    abc2=0;
    crossVul1=0;
    crossVul2=0;
    netABC1=0;
    netABC2=0;
    crossABC1=0;
    crossABC2=0;
    netVul1=0;
    netVul2=0;
    fp1=0;
    fp2=0;
}
void dfsVisit(GraphNode* u, std::map<GraphNode*, int> &color, std::map<GraphNode*,int> &d, std::map<GraphNode*,int> f, int &time, std::vector<GraphNode *>  &visitedNodesList) {
    color[u]=1; // the vertex u has discovered - color gray
    time++;
    d[u]=time;
    std::map<GraphNode*, edgeType> succs = u->getSuccessors();
    for (std::map<GraphNode*, edgeType>::iterator succ = succs.begin(), s_end = succs.end(); succ != s_end; succ++) {
        GraphNode* v = succ->first;
        DEBUG(errs()<<"dfsVisit ["<<u->getName()<<" "<<u->getLabel()<<"] color ["<<color[v]<<"] visited nodes list size=["<<visitedNodesList.size()<<"]\n");
	if(color[v]==0){ //if color=white, i.e, node has not visited yet
	     dfsVisit(v,color,d,f,time, visitedNodesList);
	}
    }
    color[u] = 2; //the vertex v has finilized  - color black
    DEBUG(errs()<<"The vertex ["<<u->getName()<<" "<<u->getLabel()<<"] has finilized and received color black color ["<<color[u]<<"] visited nodes list size=["<<visitedNodesList.size()<<"]\n");
    visitedNodesList.push_back(u);
    time++;
    f[u]=time;
}

void dfs(GraphNode *start,Graph* depGraph, std::vector<GraphNode *> &visitedNodesList){
    std::set<GraphNode*> nodes = depGraph->getNodes(); 
   
    std::map<GraphNode*, int> color;//0 - white; 1 - gray ; 2 - black 
    std::map<GraphNode*, int> discoverTime;
    std::map<GraphNode*, int> finishTime;
    for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node != end; node++) {
	color[*node] = 0;
    }
    int time = 0;
    DEBUG(errs()<<"dfs start ["<<start->getName()<<" "<<start->getLabel()<<"] Visited ["<<visitedNodesList.size()<<"\n");
    dfsVisit(start, color, discoverTime,finishTime,time,visitedNodesList);
/*
    for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node != end; node++) {
	if (color[*node] == 0){
          DEBUG(errs()<<"dfs other nodes ["<<(*node)->getName()<<" "<<(*node)->getLabel() << "]\n");
	  dfsVisit(*node, color, discoverTime,finishTime,time,visitedNodesList);
	}
    }
*/
 
}
//Count the abcs total for one array of memory nodes
int getAbcTotal(std::set<GraphNode *> memNodes){
    int abc  = 0;
    for (std::set<GraphNode*>::iterator node = memNodes.begin(), end = memNodes.end(); node != end; node++){
            MemNode *mi = dyn_cast<MemNode> (*node);
	    abc += mi->getCount();
    }
    return abc;
}
std::set<GraphNode *> counted; 
/* Receive the visited nodes list, one flag set if is cross analysis or not, and the programID of the program that is been analyzed. 
   The siot flag is used to log the ABCs considered important by SIoT 
*/
int getABC(std::vector<GraphNode *> visitedNodes1, bool cross, int programID, bool siot){
    int abc  = 0;
    for (std::vector<GraphNode*>::iterator node = visitedNodes1.begin(), end = visitedNodes1.end(); node != end; node++){
	int classId = (*node)->getClass_Id();
        //DEBUG(errs()<<"GET ABC - Calss ID["<< classId <<"]\n");
	if(counted.count(*node)==0 && classId == 4){  // 4 - Memory
	    if(cross) if((*node)->getProgramID()!=programID) continue; // in cross analysis the arrays of cross program is not the focus
            MemNode *mi = dyn_cast<MemNode> (*node);
	    abc += mi->getCount();
            counted.insert(*node);
	    GraphNode *n = *node;
            DEBUG(errs() << "Tainted Memory GraphNode " << n->getName() << " Count "<< n->getCount() << " File " <<n->getNodeCodeName() << " Line " <<n->getNodeCodeLine() <<"\n");
            if(siot) DEBUG(errs() << "SIoT_Tainted Memory GraphNode " << n->getName() << " Count "<< n->getCount() << " File " <<n->getNodeCodeName() << " Line " <<n->getNodeCodeLine() <<"\n");
	}
    }
    return abc;
}

int getVulAbc(std::set<GraphNode*> inputDeps, Graph* depGraph, std::vector< std::vector<GraphNode *> > &visitedNodesList, bool cross, int programID, bool siot){
     int abc=0;
//     counted.clear();
     for (std::set<GraphNode*>::iterator input = inputDeps.begin(), inputend = inputDeps.end(); input != inputend; ++input) {
         GraphNode *ni = *input;
         DEBUG(errs()<<"Input["<<ni->getName() <<" "<<ni->getLabel()<<"]\n");
         std::vector<GraphNode*> visitedNodes;
	 if(counted.count(ni)==0){
         	dfs(ni,depGraph,visitedNodes);
         	abc += getABC(visitedNodes, cross, programID, siot);
         	DEBUG(errs()<<"Tainted Nodes vector size["<<visitedNodes.size() <<"] ABC ["<<abc<<"] \n");
		if(abc!=0) visitedNodesList.push_back(visitedNodes);
	} else {
         	DEBUG(errs()<<"Input["<<ni->getName() <<" "<<ni->getLabel()<<"] has been visited before\n");
	}
     }
     return abc;
}

int NetVulArrays::getVul(std::set<MemNode *> memNodes1,std::set<MemNode *> inputDeps1,Graph* depGraph, std::vector< std::vector<GraphNode *> > &visitedNodesList,int* abc ){
    int vul1 = 0;
    for (std::set<MemNode *>::iterator node = memNodes1.begin(), end = memNodes1.end(); node != end; node++){
        for (std::set<MemNode *>::iterator input = inputDeps1.begin(), inputend = inputDeps1.end(); input != inputend; ++input) {
            MemNode *mi = *node;

            //DEBUG(errs()<<"Input Value["<<i->getName()<<"]\n");
            MemNode *ni = *input;
            if(mi!=NULL && ni!=NULL){
                DEBUG(errs()<<"M_Node["<<mi->getName()<<" "<<mi->getLabel()<<"]");
                DEBUG(errs()<<"Input["<<ni->getName() <<" "<<ni->getLabel()<<"]\n");
                //Graph G = depGraph->generateSubGraph(mi,1,ni,1);
                /*
                Graph G = depGraph->generateSubGraph(ni,1,mi,1);
                if(G.getNumControlEdges()!=0 || G.getNumDataEdges()!=0) vul1++;//G is not empty
                */
		std::vector<GraphNode*> vstdNodes = visitedNodesMap[std::make_pair<MemNode*, MemNode*>(ni,mi)];
 		if(vstdNodes.empty()){ 
	                std::vector<GraphNode *> visitedNodes;
       		         if(depGraph->dfsVisitVec(ni,mi, visitedNodes)){
       		         //if(visitedNodes.count(mi)) {
       		         	visitedNodesList.push_back(visitedNodes);
       		             vul1++;
			     DEBUG(errs()<<"ABCs ["<<*abc<<"] mi->getCount ["<<mi->getCount()<<"]\n");
			     *abc = *abc + mi->getCount();
			     DEBUG(errs()<<"Mem"<< mi->getName() << " " << mi->getLabel() << "Count " << mi->getCount() << " Vul["<<vul1<<"]ABCs ["<<*abc<<"]\n");
			     //DEBUG(errs()<<"Mem"<< mi->getName() << "Count " << mi->getCount() << " Vul["<<vul1<<"]\n");
       		         } else{
			     DEBUG(errs()<<"NetVulArrays: Not dfsVisitVec\n");
			 }
			 visitedNodesMap[std::make_pair<GraphNode*, GraphNode*>(ni, mi)] = visitedNodes;
		} else {
			DEBUG(errs()<<"Not dfs in NetVulArrays.  VstdNodes is not empty, size=["<<vstdNodes.size()<<"]\n");
		}

            }
        }
    }
    return vul1;

}

void NetVulArrays::setInputNode(std::set<Input *> inputDeps, int  programID, std::set<GraphNode*> &INList){

	for(std::set<Input*>::iterator inputMem = inputDeps.begin(), end = inputDeps.end();
			inputMem != end ; inputMem++)
	{
		GraphNode* memNode = depGraph->findNode((*inputMem)->getValue(),programID);
		InputNode* IN = new InputNode(*inputMem,memNode, (*inputMem)->getLabel());
		DEBUG(errs()<<"Create an InputNode label["<<(*inputMem)->getLabel()<<"\n");
		DEBUG(errs()<<"Label in InputNode label["<<IN->getLabel()<<"\n");
 		IN->connect(memNode);
		INList.insert(IN);
	}

}


bool NetVulArrays::runOnModule(Module &M) {

    clock_t begin,end;
    double elapsed_secs;
	begin = clock();
    //	InputValues &IV = getAnalysis<InputValues> ();
    InputDep &IV = getAnalysis<InputDep> ();
    //	AddStore &AS = getAnalysis<AddStore> ();
    netDepGraph &m = getAnalysis<netDepGraph> ();
    depGraph = m.depGraph;
    //	depGraph = AS.getModifiedGraph();
    DenseMap<Function*, bool> funcHasArray;
        std::set<llvm::GraphNode*> sendP1; 
        std::set<llvm::GraphNode*> sendP2;
        std::set<llvm::GraphNode*> recvP1;
        std::set<llvm::GraphNode*> recvP2; 

    std::vector< std::vector<GraphNode*> > visitedNodesList;
    std::vector< std::vector<GraphNode*> > memInputList;

    //Check MemNodes P1 -> InputDeps P1
    DEBUG(errs()<<"Check MemNodes P1 -> InputDeps P1\n");
    visitedNodesList.clear();
    std::set<GraphNode *> nodes = depGraph->getNodes();
    DEBUG(errs()<<"Nodes size="<<nodes.size()<< "\n");
    std::set<GraphNode *> memNodes1 = depGraph->getMemNodes1();
    arraysP1 = memNodes1.size();
    arraysAbcP1 = getAbcTotal(memNodes1); 
    std::set<Input *> inputDepsValue1 = IV.getInputDepValues(1);
    std::set<GraphNode*> inputDepNodes1;
    setInputNode(inputDepsValue1,1,inputDepNodes1);
    counted.clear();
    abc1 = getVulAbc(inputDepNodes1,depGraph,visitedNodesList,false,1,true);
    vul1 = counted.size();
    if (visitedNodesList.size() > 0) {
       toDotFocusDFS(visitedNodesList, BC+"_MemNodesP1-InputDepsP1");
       memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());
    }
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    DEBUG(errs()<<"P#1 - Arrays=["<< arraysP1 <<"] Vul ["<<vul1<<"] ABCs ["<<abc1<<"] Time ["<<elapsed_secs<<"]\n");

    //Check MemNodes P1 -> NetInputDeps P1
    DEBUG(errs()<<"Check MemNodes P1 -> NetInputDeps P1\n");
    visitedNodesList.clear();
    std::set<Input *> netInputDeps1 = IV.getNetInputDepValues(1);
    std::set<GraphNode*> netInputDepNodes1;
    setInputNode(netInputDeps1,1,netInputDepNodes1);
    std::set<GraphNode *> countedInput = counted; 
    counted.clear();
    netABC1 = getVulAbc(netInputDepNodes1,depGraph,visitedNodesList,false,1,false);
    DEBUG(errs()<<"Net Counted size["<< counted.size() <<"] Input Counted size ["<<countedInput.size()<<"]\n");
    for(std::set<GraphNode *>::iterator graphNode = counted.begin(), end = counted.end(); graphNode != end ; graphNode++){
        if(countedInput.count(*graphNode)) {
    		DEBUG(errs()<<"Decrements ABC because NetABC should contains only exclusive NET ABCP \n");
		netABC1 = netABC1 - (*graphNode)->getCount(); //NetABC contains only exclusive NET ABC
	}
    }
 
    netVul1=counted.size();
    if (visitedNodesList.size() > 0) {
        toDotFocusDFS(visitedNodesList, BC+"_MemNodesP1-NetInputDepsP1");
        memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());
    }

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    DEBUG(errs()<<"P#1 - netInputDeps1 size["<< netInputDeps1.size()<<"]\n");
    DEBUG(errs()<<"P#1 - Arrays=["<< arraysP1 <<"] NetVul ["<<netVul1<<"] NetABC ["<<netABC1<<"] Time ["<<elapsed_secs<<"]\n");

    //Check MemNodes P1 -> InputDeps P2
    DEBUG(errs()<<"Check MemNodes P1 -> InputDeps P2\n");
    visitedNodesList.clear();
    //std::set<Input *> inputDepsValue2 = IV.getInputDepValuesWithNetSources(2);
    std::set<Input *> inputDepsValue2 = IV.getInputDepValues(2);
    std::set<GraphNode*> inputDepNodes2;
    setInputNode(inputDepsValue2,2,inputDepNodes2);
    counted.clear();
    crossABC1 = getVulAbc(inputDepNodes2,depGraph,visitedNodesList,true,1,true);
    crossVul1 = counted.size();
    if (visitedNodesList.size() > 0) {
        toDotFocusDFS(visitedNodesList, BC+"_MemNodesP1-InputDepsP2");
        memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());
    }
    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    DEBUG(errs()<<"P#1 - Arrays=["<< memNodes1.size()<<"] CrossVul ["<<crossVul1<<"] CrossABC ["<<crossABC1<<"] Time ["<<elapsed_secs<<"]\n");

    //Calculate False Positives
    fp1 = netVul1 - crossVul1;
    int falseABC1 = netABC1 -crossABC1; 
    DEBUG(errs()<<"P#1 - Arrays=["<< arraysP1 <<"] False Positive ["<< fp1 <<"] Unnecessary ABCs ["<<falseABC1<<"\n");

    //Check MemNodes P2 -> InputDeps P2
    DEBUG(errs()<<"Check MemNodes P2 -> InputDeps P2\n");
    visitedNodesList.clear();
    std::set<GraphNode *> memNodes2 = depGraph->getMemNodes2();
    arraysP2 = memNodes2.size();
    arraysAbcP2 = getAbcTotal(memNodes2); 
    inputDepsValue2 = IV.getInputDepValues(2);
    inputDepNodes2.clear();
    setInputNode(inputDepsValue2,2,inputDepNodes2);
    //vul2 = getVul(memNodes2,inputDepNodes2,depGraph, memNodesP2InputDepsP2List);
    counted.clear();
    abc2 = getVulAbc(inputDepNodes2,depGraph,visitedNodesList,false,2,true);
    vul2 = counted.size();
    if (visitedNodesList.size() > 0) {
    	toDotFocusDFS(visitedNodesList, BC+"_MemNodesP2-InputDepsP2");
        memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());
    }
    DEBUG(errs()<<"P#2 - Arrays=["<< arraysP2 <<"] Vul ["<<vul2<<"] ABC ["<<abc2<<"]\n");

    //Check MemNodes P2 -> NetInputDeps P2
    DEBUG(errs()<<"Check MemNodes P2 -> NetInputDeps P2\n");
    visitedNodesList.clear();
    std::set<Input *> netInputDeps2 = IV.getNetInputDepValues(2);
    std::set<GraphNode*> netInputDepNodes2;
    setInputNode(netInputDeps2,2,netInputDepNodes2);
    countedInput = counted;
    counted.clear();
    netABC2 = getVulAbc(netInputDepNodes2,depGraph,visitedNodesList,false,2,false);
    DEBUG(errs()<<"Net Counted size["<< counted.size() <<"] Input Counted size ["<<countedInput.size()<<"]\n");
    for(std::set<GraphNode *>::iterator graphNode = counted.begin(), end = counted.end(); graphNode != end ; graphNode++){
        if(countedInput.count(*graphNode)) {
                DEBUG(errs()<<"Decrements ABC because NetABC should contains only exclusive NET ABC \n");
                netABC2 = netABC2 - (*graphNode)->getCount(); //NetABC contains only exclusive NET ABC
        }
    }
    netVul2 = counted.size();
    if (visitedNodesList.size() > 0) {
    	toDotFocusDFS(visitedNodesList, BC+"_MemNodesP2-NetInputDepsP2");
        memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());
    }
    DEBUG(errs()<<"P#2 - netInputDeps2 size["<< netInputDeps2.size()<<"]\n");
    DEBUG(errs()<<"P#2 - Arrays=["<< arraysP2 <<"] NetVul ["<<netVul2<<"] NetABC2 ["<<netABC2<<"]\n");

    //Check MemNodes P2 -> InputDeps P1
    DEBUG(errs()<<"Check MemNodes P2 -> InputDeps P1\n");
    visitedNodesList.clear();
    inputDepsValue1 = IV.getInputDepValues(1);
    inputDepNodes1.clear();
    setInputNode(inputDepsValue1,1,inputDepNodes1);
    counted.clear();
    crossABC2 = getVulAbc(inputDepNodes1,depGraph,visitedNodesList,true,2,true);
    crossVul2 = counted.size();

    if (visitedNodesList.size() > 0) {
        toDotFocusDFS(visitedNodesList, BC+"_MemNodesP2-InputDepsP1");
        memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());
    }
    DEBUG(errs()<<"P#2 - Arrays=["<< memNodes2.size()<<"] CrossVul ["<<crossVul2<<"] CrossABC ["<<crossABC2<<"]\n");

    //Calculate False Positives
    int falseABC2 = netABC2 -crossABC2; 
    fp2 = netVul2 - crossVul2;
    DEBUG(errs()<<"P#2 - Arrays=["<< memNodes2.size()<<"] False Positive ["<<netVul2-crossVul2<<"] Unnecessary ABCs["<<falseABC2<<"]\n");

    end = clock();
    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

    //Save statistic file

    std::string name = BC;

    std::string statFileName ="NetVulArrays_statistics.txt";
    ifstream infile(statFileName.c_str());

    if(!infile.good()){
                ofstream File(statFileName.c_str(),std::ios_base::out | std::ios_base::app);
                File<<"Arr1; Vul1; VulCross1; Net1; SecCross1; TotalABC1; ABC1; CrossABC1; NetABC1; FalseABC1; Arr2; Vul2; VulCross2; Net2; SecCross2; TotalABC2; ABC2; CrossABC2; NetABC2; FalseABC2; Time(s); Serv_ClientName" << endl; 
                File.close();
    }
    ofstream File(statFileName.c_str(),std::ios_base::out | std::ios_base::app);

    string buffer;
    File << arraysP1.getValue() << ";\t";
    File << vul1.getValue() << ";\t";
    File << crossVul1.getValue() << ";\t";
    File << netVul1.getValue() << ";\t";
    File << fp1.getValue() << ";\t";
    File << arraysAbcP1.getValue() << ";\t";
    File << abc1.getValue() << ";\t";
    File << crossABC1.getValue() << ";\t";
    File << netABC1.getValue() << ";\t";
    File << falseABC1 << ";\t";
    File << arraysP2.getValue() << ";\t";
    File << vul2.getValue() << ";\t";
    File << crossVul2.getValue() << ";\t";
    File << netVul2.getValue() << ";\t";
    File << fp2.getValue() << ";\t";
    File << arraysAbcP2.getValue() << ";\t";
    File << abc2.getValue() << ";\t";
    File << crossABC2.getValue() << ";\t";
    File << netABC2.getValue() << ";\t";
    File << falseABC2 << ";\t";
    File << elapsed_secs << ";\t";
    File << name  << endl;
    File.close();

    //Generate dot file
    std::string ErrorInfo("");
    std::string dotFileName = name + "_sendRecvConnections.dot";
    raw_fd_ostream FileDot1(dotFileName.c_str(), ErrorInfo);

//    if (ErrorInfo.compare("")) {
//    	errs () << "[VulArrays]  " << ErrorInfo << "\n";
//    }
//    else

	depGraph->toDotFocusSendRecv(dotFileName.c_str(), &FileDot1, name); //, guider);

    dotFileName = name + "_sendRecvConnectionsWithMemAndInputNodes.dot";
    raw_fd_ostream FileDot2(dotFileName.c_str(), ErrorInfo);


/*
    std::vector< std::vector<GraphNode*> > memNodesP1InputDepsP1PlusmemNodesP2InputDepsP2List;
    memNodesP1InputDepsP1PlusmemNodesP2InputDepsP2List.reserve(memNodesP1InputDepsP1List.size()+memNodesP2InputDepsP2List.size());
    memNodesP1InputDepsP1PlusmemNodesP2InputDepsP2List.insert(memNodesP1InputDepsP1PlusmemNodesP2InputDepsP2List.end(),memNodesP1InputDepsP1List.begin(),memNodesP1InputDepsP1List.end());
    memNodesP1InputDepsP1PlusmemNodesP2InputDepsP2List.insert(memNodesP1InputDepsP1PlusmemNodesP2InputDepsP2List.end(),memNodesP2InputDepsP2List.begin(),memNodesP2InputDepsP2List.end());
*/
    std::set<GraphNode*> netSet = depGraph->getNetReadNodes(1);
    std::vector<GraphNode*> visitedNodes; 
    std::copy(netSet.begin(), netSet.end(), std::back_inserter(visitedNodes));
    DEBUG(errs()<<"Net Read Nodes program 1="<<netSet.size()<<"\n");
    visitedNodesList.push_back(visitedNodes); 
    memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());

    netSet = depGraph->getNetReadNodes(2);
    DEBUG(errs()<<"Net Read Nodes program 2="<<netSet.size()<<"\n");
    std::copy(netSet.begin(), netSet.end(), std::back_inserter(visitedNodes));
    visitedNodesList.push_back(visitedNodes); 
    memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());

    netSet = depGraph->getNetWriteNodes(1);
    DEBUG(errs()<<"Net Write Nodes program 1="<<netSet.size()<<"\n");
    std::copy(netSet.begin(), netSet.end(), std::back_inserter(visitedNodes));
    visitedNodesList.push_back(visitedNodes); 
    memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());

    netSet = depGraph->getNetWriteNodes(2);
    std::copy(netSet.begin(), netSet.end(), std::back_inserter(visitedNodes));
    visitedNodesList.push_back(visitedNodes); 
    memInputList.insert(memInputList.end(),visitedNodesList.begin(),visitedNodesList.end());

    DEBUG(errs()<<"MemInputList size "<<memInputList.size()<<" dotFileName[" <<dotFileName.c_str()<<"\n");
    depGraph->toDotFocusSendRecvWithMemAndInputNodes(memInputList, dotFileName.c_str(), &FileDot2, name);
    toDotFocusDFS(memInputList, dotFileName.c_str());
	dotFileName = name + "_netdepgraph.dot";
	raw_fd_ostream DepGraphFile(dotFileName.c_str(), ErrorInfo);
	depGraph->toDot("main", &DepGraphFile);
	DEBUG(errs() << "DepGraph toDot done\n");
	// Create an output archive
	depGraph->saveToFile(name+"_netsaved.txt");
			DEBUG(errs() << "DepGraph saveToFile done\n");
	printStats();
	depGraph->saveStatisticsToFile(name);

    //toDot(M.getModuleIdentifier());
    return false;

    //	printStats();
    //	printArrays();
    //return false;

}

void NetVulArrays::toDotFocusDFS(std::vector<std::vector<GraphNode*> > visitedNodesList,std::string fileName) {
	Graph::Guider* guider = new Graph::Guider(depGraph);
	std::vector<GraphNode*> visitedNodes;
	std::map<std::string, GraphNode*> visitedNodesComplete;
	std::map<std::string,std::string> visitedNodesRelations;
	std::string relation;
	std::string ErrorInfo("");
	std::string color;
	std::string label;
	StringRef graphFlag = GRAPH;
	
	if(graphFlag.equals("-1")) return;

	fileName += "Focus.dot";

	raw_fd_ostream File2(fileName.c_str(), ErrorInfo);

	(File2) << "digraph \"DFG for \'" << fileName << "\' function \"{\n";
	(File2) << "label=\"DFG for \'" << fileName << "\' function\";\n";

	GraphNode *previous;

	for (int i = 0; i < visitedNodesList.size(); i++) {
				visitedNodes = visitedNodesList[i];
				if(graphFlag.equals("0"))
				{
					stringstream s;
					s.clear();
					s << i;
					string num = s.str();
					(File2) << "subgraph " << num << " { \n";
				}

				for (std::vector<GraphNode*>::iterator node = visitedNodes.begin(), end =
						visitedNodes.end(); node != end; node++) {

					if(node == visitedNodes.begin())
						previous = *node;
					stringstream s;
					s << (*node)->getLabel() << " " << (*node)->getName() << " ABC=" <<(*node)->getCount()  ;
					label = s.str();

					color = (*node)->getColor();
					
					DEBUG(errs()<<"[toDotFocusDFS] Label ["<< label<<"] ");
					bool focus = false;
					int classId = (*node)->getClass_Id();
					DEBUG(errs()<<"Class ID["<< classId <<"]\n");
					focus = (classId == 4) || (classId == 5) || (classId == 6) || (classId == 7); // 4 - Memory, 5 - NetRead, 6 - NetWrite, 7 - Input  

					if(focus || (node == visitedNodes.begin()))
					{
						DEBUG(errs()<<"Name["<< (*node)->getName() <<"]\n");
						if(visitedNodesComplete.count((*node)->getName()) == 0)
						{
							DEBUG(errs() << "\"" << (*node)->getName() << "\"" << "[label=\"" << label + "\" color=\"" << color << "\", style=filled]\n");
							(File2) << "\"" << (*node)->getName() << "\"" << "[label=\"" << label + "\" color=\"" << color << "\", style=filled]"; //label << program << "\"" << "[label=\"" << label + "\" color=\"" << color << "\", style=filled]";
							//(File2) << "\"" << (*node)->getName() << "\"" ; //label << program << "\"" << "[label=\"" << label + "\" color=\"" << color << "\", style=filled]";
							(File2) << "\n";


							

							visitedNodesComplete[(*node)->getName()] = *node;

						}
					}


					if(node != visitedNodes.begin())
					{
						if(focus)
						{
							relation = "\"" + previous->getName() + "\"";
							relation += "->";
							relation += "\"" + (*node)->getName() + "\"";
							relation += "\n";

							previous = *node;

							if(graphFlag.equals("1"))
							{
								if(visitedNodesRelations.count(relation) == 0)
								{
									visitedNodesRelations[relation] = relation;
									(File2) << relation;
								}
							}
							else if(graphFlag.equals("0"))
							{
								(File2) << relation;
							}
						}
					}
				}
				if(graphFlag.equals("0"))
				{
					(File2) << "}\n\n";
				}

	}

	(File2) << "}\n\n";

}


bool isSendOrRecvNode(GraphNode *node){
	return (node->getClass_Id()==5||node->getClass_Id()==6);
}

void NetVulArrays::toDot(std::string name) {
    Graph::Guider* guider = new Graph::Guider(depGraph);
    std::string ErrorInfo("");
    std::string fileName = "net.vul.dot";
    std::string s;
    std::ostringstream ss;
    for (DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > >::iterator
         i = VulArrays1.begin(), e = VulArrays1.end(); i != e; ++i) {
        s = "[label=\"";
        s += i->first->getLabel() + "\" color=\"#FF6161\", style=filled]";
        guider->setNodeAttrs(i->first, s);
        i->first->setFocus(true);
       // i->first->setColor("\"#FF6161\"");
        DenseMap<const Value*, std::vector<GraphNode*> >::iterator ii =
                i->second.begin(), ee = i->second.end();
        if (VulArrays1.count(ii->second.front()))
            continue; // node should be red, not yellow
        s = "[label=\"";
        s += ii->second.front()->getLabel()
                + "\" color=\"#FFFF7A\", style=filled]";
        guider->setNodeAttrs(ii->second.front(), s);
        ii->second.front()->setFocus(true);
        //ii->second.front()->setColor("\"#FFFF7A\"");
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
        i->first->setFocus(true);
        //i->first->setColor("\"#FF6161\"");

        DenseMap<const Value*, std::vector<GraphNode*> >::iterator ii =
                i->second.begin(), ee = i->second.end();
        if (Structs1.count(ii->second.front()))
            continue; // node should be red, not yellow
        s = "[label=\"";
        s += ii->second.front()->getLabel()
                + "\" color=\"#FFFF7A\", style=filled]";
        guider->setNodeAttrs(ii->second.front(), s);
        ii->second.front()->setFocus(true);
        //ii->second.front()->setColor("\"#FFFF7A\"");
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
        depGraph->toDot(name, &File);//, guider);
    printStats();
}

void NetVulArrays::printStats() {
    int count;
    double sum;
    for (DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > >::iterator
         i = VulArrays1.begin(), e = VulArrays1.end(); i != e; ++i) {
        errs() << "[VulArrays] ";
        count = 0;
        sum = 0;
        for (DenseMap<const Value*, std::vector<GraphNode*> >::iterator ii =
             i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {
            ++count;
            sum += ii->second.size() - 1;
        }
        errs() << sum / count << "\n";
    }
    errs() << "[VulArrays] ***********************************\n";
    for (DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > >::iterator
         i = Structs1.begin(), e = Structs1.end(); i != e; ++i) {
        errs() << "[VulArrays] ";
        count = 0;
        sum = 0;
        for (DenseMap<const Value*, std::vector<GraphNode*> >::iterator ii =
             i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {
            ++count;
            sum += ii->second.size() - 1;
        }
        errs() << sum / count << "\n";
    }
}

void NetVulArrays::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<netDepGraph> ();
    //	AU.addRequired<AddStore> ();
    AU.addRequired<InputValues> ();
    AU.addRequired<InputDep> ();
}

char NetVulArrays::ID = 0;
static RegisterPass<NetVulArrays> X("netVulArrays",
                                    "Vulnerability pass: identify arrays that cause vulnerabilities");
