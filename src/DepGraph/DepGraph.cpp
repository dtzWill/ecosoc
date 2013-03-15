#define DEBUG_TYPE "depgraph"

#include "DepGraph.h"

using namespace llvm;

//*********************************************************************************************************************************************************************
//																DEPENDENCE GRAPH API
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






/*
 * Class GraphNode
 */

GraphNode::GraphNode(){
	Class_ID = 0;
	ID = currentID++;
}

GraphNode::~GraphNode (){

	for( std::set<GraphNode*>::iterator pred = predecessors.begin(); pred != predecessors.end(); pred++ ) {
		(*pred)->successors.erase(this);
	}

	for( std::set<GraphNode*>::iterator succ = successors.begin(); succ != successors.end(); succ++ ) {
		(*succ)->predecessors.erase(this);
	}

	successors.clear();
	predecessors.clear();
}

std::set<GraphNode*> llvm::GraphNode::getSuccessors() {
	return successors;
}

std::set<GraphNode*> llvm::GraphNode::getPredecessors() {
	return predecessors;
}

void llvm::GraphNode::connect(GraphNode* dst){
	this->successors.insert(dst);
	dst->predecessors.insert(this);
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

std::string llvm::GraphNode::getName(){
	std::ostringstream stringStream;
	stringStream << "node_" << getId();
	return stringStream.str();
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



/*
 * Class CallNode
 */
Function* llvm::CallNode::getCalledFunction() const {
	return F;
}

std::string llvm::CallNode::getLabel() {
	std::ostringstream stringStream;
	stringStream << "Call " << F->getName().str();
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
Value* VarNode::getValue() {
	return value;
}

std::string llvm::VarNode::getShape() {

	if (!isa<Constant>(value)) {
		return std::string("ellipse");
	} else {
		return std::string("box");
	}

}

std::string llvm::VarNode::getLabel() {

	std::ostringstream stringStream;

	if (!isa<Constant>(value)) {

		stringStream << value->getName().str();

	} else {

		if ( ConstantInt* CI = dyn_cast<ConstantInt>(value)) {
			stringStream << CI->getValue().toString(10,true);
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
 * Class Graph
 */
Graph::~Graph () {
	nodes.clear();
}

Graph Graph::generateSubGraph (Value *src, Value *dst){
	Graph G;

	std::map<GraphNode*, GraphNode*> nodeMap;

	std::set<GraphNode*> visitedNodes1;
	std::set<GraphNode*> visitedNodes2;


	GraphNode* source = findNode(src);
	GraphNode* destination = findNode(dst);

	if (source == NULL || destination == NULL) {
		return G;
	}


	dfsVisit(source, visitedNodes1);
	dfsVisitBack(destination, visitedNodes2);

	//check the nodes visited in both directions
	for (std::set<GraphNode*>::iterator it=visitedNodes1.begin(); it!=visitedNodes1.end(); ++it){
		if(visitedNodes2.count(*it) > 0) {
			nodeMap[*it] = (*it)->clone();
		}
	}

	//connect the new vertices
	for (std::map<GraphNode*, GraphNode*>::iterator it=nodeMap.begin(); it!=nodeMap.end(); ++it) {

		std::set<GraphNode*> succs = it->first->getSuccessors();

		for (std::set<GraphNode*>::iterator succ = succs.begin(), s_end = succs.end(); succ != s_end; succ++){
			if ( nodeMap.count(*succ) != 0  ) {

				it->second->connect( nodeMap[*succ] );

			}
		}

		G.nodes.insert(it->first);

	}

	return G;
}

void Graph::dfsVisit (GraphNode* u, std::set<GraphNode*> &visitedNodes){

	visitedNodes.insert(u);

	std::set<GraphNode*> succs = u->getSuccessors();

	for (std::set<GraphNode*>::iterator succ = succs.begin(), s_end = succs.end(); succ != s_end; succ++){
		if ( visitedNodes.count(*succ) == 0  ) {
			dfsVisit(*succ, visitedNodes);
		}
	}

}

void Graph::dfsVisitBack (GraphNode* u, std::set<GraphNode*> &visitedNodes){

	visitedNodes.insert(u);

	std::set<GraphNode*> preds = u->getPredecessors();

	for (std::set<GraphNode*>::iterator pred = preds.begin(), s_end = preds.end(); pred != s_end; pred++){
		if ( visitedNodes.count(*pred) == 0  ) {
			dfsVisitBack(*pred, visitedNodes);
		}
	}

}


//Print the graph (.dot format) in the stderr stream.
void Graph::toDot (std::string s) {

	this->toDot(s, &errs());

}


void Graph::toDot (std::string s, const std::string fileName){

	std::string ErrorInfo;

	raw_fd_ostream File(fileName.c_str(), ErrorInfo);

	if (!ErrorInfo.empty()){
	  errs() << "Error opening file " << fileName << " for writing! Error Info: " << ErrorInfo  << " \n";
	  return;
	}

	this->toDot(s, &File);

}

void Graph::toDot (std::string s, raw_ostream *stream) {

	(*stream)<<"digraph \"DFG for \'"<< s <<"\' function \"{\n";
	(*stream)<< "label=\"DFG for \'"<< s <<"\' function\";\n";

	std::map<GraphNode*,int> DefinedNodes;


	for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node != end; node++){

		if (DefinedNodes.count(*node) == 0) {
			(*stream) << (*node)->getName() << "[shape=" << (*node)->getShape() << ",label=\"" <<  (*node)->getLabel() << "\"]\n";
			DefinedNodes[*node] = 1;
		}


		std::set<GraphNode*> succs = (*node)->getSuccessors();

		for (std::set<GraphNode*>::iterator succ = succs.begin(), s_end = succs.end(); succ != s_end; succ++){

			if (DefinedNodes.count(*succ) == 0) {
				(*stream) << (*succ)->getName() << "[shape=" << (*succ)->getShape() << ",label=\"" <<  (*succ)->getLabel() << "\"]\n";
				DefinedNodes[*succ] = 1;
			}

			//Source
			(*stream)<<"\"" << (*node)->getName() << "\"";

			(*stream)<<"->";

			//Destination
			(*stream)<<"\"" << (*succ)->getName() << "\"";

			(*stream)<<"\n";

		}

	}

	(*stream)<<"}\n\n";

}

GraphNode* Graph::addInst (Value *v)  {

	GraphNode *Op, *Var, *Operand;

	if (isValidInst(v)) { //If is a data manipulator instruction
		Var = this->findNode(v);
		if (Var == NULL) { //If it has not processed yet

			Var = new VarNode(v);
			nodes.insert(Var);

			if (isa<Instruction>(v)) {

				if( CallInst* CI = dyn_cast<CallInst>(v)) {
					Op = new CallNode(CI->getCalledFunction());
				} else {
					Op = new OpNode(dyn_cast<Instruction>(v)->getOpcode());
				}
				nodes.insert(Op);
				Op->connect(Var);

				//Connect the operands to the OpNode
				for (unsigned int i=0; i<cast<User>(v)->getNumOperands(); i++) {
					Value *v1 = cast<User>(v)->getOperand(i);
					Operand = this->addInst(v1);

					if (Operand != NULL) Operand->connect(Op);
				}
			}
		}

		return Var;
	}
	return NULL;
}

void Graph::addEdge(GraphNode* src, GraphNode* dst){

	nodes.insert(src);
	nodes.insert(dst);
	src->connect(dst);

}


//It verify if the instruction is valid for the dependence graph, i.e. just data manipulator instructions are important for dependence graph
bool Graph::isValidInst(Value *v) {
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
	else if (isa<Constant>(v) && !isa<Function>(v)) {
			return true;
		}
	else if (isa<Argument>(v) ) {
			return true;
	} else {
			return false;
	}
}


//Return the pointer to the node related to the operand.
//Return NULL if the operand is not inside map.
GraphNode* Graph::findNode (Value *op){

	for (std::set<GraphNode*>::iterator i = nodes.begin(), vend = nodes.end(); i != vend; ++i) {
		if (isa<VarNode>(*i) && dyn_cast<VarNode>(*i)->getValue() == op ){
			return dyn_cast<VarNode>(*i);
		}
	}

	return NULL;
}

void llvm::Graph::print() {

	for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node != end; node++){

		errs() << "Node ID:" << (*node)->getId() << " Successors:";
		std::set<GraphNode*> succs = (*node)->getSuccessors();
		for (std::set<GraphNode*>::iterator succ = succs.begin(), s_end = succs.end(); succ != s_end; succ++){
			errs() << (*succ)->getId() <<", ";
		}

		errs() << " Predecessors:";
		std::set<GraphNode*> preds = (*node)->getPredecessors();
		for (std::set<GraphNode*>::iterator pred = preds.begin(), s_end = preds.end(); pred != s_end; pred++){
			errs() << (*pred)->getId() <<", ";
		}

		errs() << "\n";

	}

	errs() << "\n";

}

void llvm::Graph::deleteCallNodes(Function* F) {

	for (std::set<GraphNode*>::iterator node = nodes.begin(), end = nodes.end(); node != end; node++){

		if (CallNode* CN = dyn_cast<CallNode>(*node)) {
			if (CN->getCalledFunction() == F) {
				nodes.erase(node);
				delete CN;
			}
		}
	}
}

//*********************************************************************************************************************************************************************
//																DEPENDENCE GRAPH CLIENT
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
	AU.setPreservesAll();
}

bool functionDepGraph::runOnFunction(Function &F) {

    //Making dependency graph
	depGraph = new llvm::Graph();
	//Insert instructions in the graph
	for (Function::iterator BBit = F.begin(), BBend = F.end(); BBit != BBend; ++BBit) {
		for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; ++Iit) {
			depGraph->addInst(Iit);
		}
	}

	//We don't modify anything, so we must return false
    return false;
}

char functionDepGraph::ID = 0;
static RegisterPass<functionDepGraph> X("functionDepGraph", "Function Dependence Graph");




//Class moduleDepGraph
void moduleDepGraph::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
}

bool moduleDepGraph::runOnModule(Module &M) {

    //Making dependency graph
	depGraph = new Graph();

	//Insert instructions in the graph
	for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit){
		for (Function::iterator BBit = Fit->begin(), BBend = Fit->end(); BBit != BBend; ++BBit) {
			for (BasicBlock::iterator Iit = BBit->begin(), Iend = BBit->end(); Iit != Iend; ++Iit) {
				depGraph->addInst(Iit);
			}
		}
	}


	//Connect formal and actual parameters and return values
	for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend; ++Fit){

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
	if (!F.hasNUsesOrMore(1)) {
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
	SmallPtrSet<Value*, 8> ReturnValues;

	if (!noReturn) {
		// Iterate over the basic blocks to fetch all possible return values
		for (Function::iterator bb = F.begin(), bbend = F.end(); bb != bbend;
				++bb) {
			// Get the terminator instruction of the basic block and check if it's
			// a return instruction: if it's not, continue to next basic block
			Instruction *terminator = bb->getTerminator();

			ReturnInst *RI = dyn_cast<ReturnInst>(terminator);

			if (!RI)
				continue;

			// Get the return value and insert in the data structure
			ReturnValues.insert(RI->getReturnValue());
		}
	}


	for (Value::use_iterator UI = F.use_begin(), E = F.use_end(); UI != E;
			++UI) {
		User *U = *UI;

		// Ignore blockaddress uses
		if (isa<BlockAddress>(U))
			continue;

		// Used by a non-instruction, or not the callee of a function, do not
		// match.
		if (!isa<CallInst>(U) && !isa<InvokeInst>(U))
			continue;

		Instruction *caller = cast<Instruction>(U);

		CallSite CS(caller);
		if (!CS.isCallee(UI))
			continue;

		// Iterate over the real parameters and put them in the data structure
		CallSite::arg_iterator AI;
		CallSite::arg_iterator EI;

		for (i = 0, AI = CS.arg_begin(), EI = CS.arg_end(); AI != EI; ++i, ++AI)
			Parameters[i].second = depGraph->addInst(*AI);


		// Match formal and real parameters
		for (i = 0; i < Parameters.size(); ++i) {
			depGraph->addEdge(Parameters[i].second, Parameters[i].first);
		}

		// Match return values
		if (!noReturn) {

			OpNode* retPHI = new OpNode(Instruction::PHI);
			GraphNode* callerNode = depGraph->addInst(caller);
			depGraph->addEdge(retPHI, callerNode);

			for (SmallPtrSetIterator<Value*> ri = ReturnValues.begin(), re =
					ReturnValues.end(); ri != re; ++ri) {
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

char moduleDepGraph::ID = 0;
static RegisterPass<moduleDepGraph> Y("moduleDepGraph",
		"Module Dependence Graph");


