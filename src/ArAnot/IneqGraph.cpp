#define DEBUG_TYPE "ineq-graph"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "RangeAnalysis.h"
#include "IneqGraph.h"

#include <queue>

using namespace llvm;

using std::queue;

GraphEdge::GraphEdge() :
  From(NULL), To(NULL), ETy(NONE) {
}

GraphEdge::GraphEdge(GraphNode *From, GraphNode *To, EdgeType ETy, APInt Weight) :
  From(From), To(To), ETy(ETy), Weight(Weight) {
}

void GraphEdge::print(raw_ostream &OS) {
  const StringRef MayColor = "black";
  const StringRef MustColor = "blue";

  StringRef Color;
  if (ETy == MAY) {
    Color = MayColor;
  } else if (ETy == MUST) {
    Color = MustColor;
  } else {
    assert(false && "Invalid ETy");
  }

  Value *FromValue = From->getValue();
  if (FromValue->hasName())
    OS << "    \"" << FromValue->getName();
  else
    OS << "    \"" << *FromValue;

  OS << "\" -> "; 

  Value *ToValue = To->getValue();
  if (ToValue->hasName())
    OS << "\"" << ToValue->getName();
  else
    OS << "\"" << *ToValue;

  OS << "\" [label =\"" << Weight << "\", color = \"" << Color << "\"];\n";
}

// getOrCreateNode
GraphNode *IneqGraph::getOrCreateNode(Value *V) {
  if (!NodeMap.count(V))
    NodeMap[V] = new GraphNode(V);
  return NodeMap[V];
}

// getNode
GraphNode *IneqGraph::getNode(Value *V) {
  return NodeMap.count(V) ? NodeMap[V] : NULL;
}

/* void GraphNode::getReachableNodes(SetVector<GraphNode*> &Mark) {
  queue<GraphNode*> Queue;
  Queue.push(this);

  Mark.insert(this);

  while (!Queue.empty()) {
    GraphNode *Front = Queue.front();
    Queue.pop();
    for (may_iterator It = Front->may_begin(), E = Front->may_end();
         It != E; ++It)
      if (Mark.insert(It->getToEdge()))
        Queue.push(It->getToEdge());
  }
}

void GraphNode::intersect(SetVector<GraphNode*> &Mark, SetVector<GraphNode*> &Other) {
  SetVector<GraphNode*> Erase;
  for (SetVector<GraphNode*>::iterator It = Mark.begin(), E = Mark.end();
       It != E; ++It) {
    if (!Other.count(*It))
      Erase.insert(*It);
  }
  for (SetVector<GraphNode*>::iterator It = Erase.begin(), E = Erase.end();
       It != E; ++It) {
    Mark.remove(*It);
  }
}


void GraphNode::removeMustEdges() {
  SetVector<GraphNode*> *Intersection = NULL;
  bool Initialized = false;
  for (must_iterator It = must_begin(), E = must_end();
       It != E; ++It) {
    SetVector<GraphNode*> Reachable;
    It->getToEdge()->getReachableNodes(Reachable);
    if (!Initialized) {
      Initialized = true;
      Intersection = new SetVector<GraphNode*>(Reachable.begin(), Reachable.end());
    } else {
      intersect(*Intersection, Reachable);
    }
  }
  MustEdges.clear();
  //minimize(Intersection);
} */

APInt GraphNode::findShortestPath(GraphNode *Dest, SetVector<GraphNode*> Visited) {
  if (Dest == this) {
    DEBUG(dbgs() << "IneqGraph: Reached: " << *Dest->getValue() << "\n");
    return APInt(64, 0, true);
  }

  DEBUG(dbgs() << "IneqGraph: Node: " << *V << "\n");

  if (Visited.count(this)) {
    DEBUG(dbgs() << "IneqGraph: Visited\n");
    return APInt::getSignedMaxValue(64); //(64, , true);
  }
  Visited.insert(this);

  APInt MayMin = APInt::getSignedMaxValue(64);
  for (may_iterator It = may_begin(), E = may_end();
       It != E; ++It) {
    APInt Total = It->getToEdge()->findShortestPath(Dest, Visited);
    if (Total.slt(MayMin))
      MayMin = Total + It->getWeight();
  }
  //DEBUG(dbgs() << "IneqGraph: Node: " << *V << ", MayMin: " << MayMin << "\n");

  APInt MustMax = APInt::getSignedMinValue(64);
  for (must_iterator It = must_begin(), E = must_end();
       It != E; ++It) {
    APInt Total = It->getToEdge()->findShortestPath(Dest, Visited);
    if (MustMax.slt(Total))
      MustMax = Total;
  }
 // DEBUG(dbgs() << "IneqGraph: Node: " << *V << ", MustMax: " << MustMax << "\n");

  if (MustMax.isMinSignedValue()) {
    //DEBUG(dbgs() << "IneqGraph: Node: " << *V << ", Distance: " << MayMin << "\n");
    DEBUG(dbgs() << "IneqGraph: Ret: " << MayMin << "\n");
    return MayMin;
  } else {
    APInt Min = MustMax.slt(MayMin) ? MustMax : MayMin;
    //DEBUG(dbgs() << "IneqGraph: Node: " << *V << ", Distance: " << Min << "\n");
    DEBUG(dbgs() << "IneqGraph: Ret: " << Min << "\n");
    return Min;
  }
}

/* APInt GraphNode::findShortestPath(GraphNode *Dest) {
  const APInt PlusInf = APInt::getSignedMaxValue(64);
  const APInt AlphaInt = APInt(64, 0, true);

  DenseMap<GraphNode*, APInt> Distance;
  Distance.insert(std::make_pair(this, AlphaInt));

  queue<GraphNode*> Queue;
  Queue.push(this);

  SetVector<GraphNode*> Enqueued;
  Enqueued.insert(this);

  while (!Queue.empty()) {
    GraphNode *Front = Queue.front();
    Queue.pop();
    Enqueued.remove(Front);
    APInt FrontDistance = Distance.count(Front) ? Distance[Front] : PlusInf;

    // Check for a path on the 'may' edges.
    for (may_iterator It = Front->may_begin(), E = Front->may_end();
         It != E; ++It) {
      GraphNode *Node = It->getToEdge();
      APInt ItDistance = Distance.count(Node) ? Distance[Node] : PlusInf;
      APInt ItWeight = It->getWeight();
      APInt NewDistance = FrontDistance + ItWeight;

      if (NewDistance.slt(ItDistance.getZExtValue())) {
        Distance[Node] = NewDistance;

        if (Enqueued.insert(Node)) {
          Queue.push(Node);
        }
      }
    }
  }

  return Distance.count(Dest) ? Distance[Dest] : PlusInf;
} */

// addMayEdge
// Adds the edge F -> T with weight Weight.
void IneqGraph::addMayEdge(Value *F, Value *T, APInt Weight) {
  DEBUG(dbgs() << "IneqGraph: addMayEdge: " << *F << " == " << Weight
                << " ==> " << *T << "\n");
  GraphNode *FN = getOrCreateNode(F);
  GraphNode *TN = getOrCreateNode(T);
  FN->addMayEdgeTo(TN, Weight);
}

// Adds the edge F -> T with weight Weight.
APInt IneqGraph::findShortestPath(Value *F, Value *T) {
  DEBUG(dbgs() << "IneqGraph: findShortestPath: " << *F << " ==> " << *T << "\n");
  if (isa<ConstantInt>(F) || isa<ConstantInt>(T)) {
    APInt RangeF = getUpper(F);
    APInt RangeT = getLower(T);
    DEBUG(dbgs() << "Ranges: " << RangeF << " and " << RangeT << "\n");
    if (RangeF.isMaxSignedValue() || RangeF.isMinSignedValue() ||
        RangeT.isMaxSignedValue() || RangeT.isMinSignedValue())
      return APInt::getSignedMaxValue(64);
    return RangeF.sextOrSelf(64) - RangeT.sextOrSelf(64);
  }
  GraphNode *FN = getOrCreateNode(F);
  GraphNode *TN = getOrCreateNode(T);
  return FN->findShortestPath(TN);
}

APInt IneqGraph::findShortestPathAt(Value *F, Value *T, BasicBlock *BB) {
  DEBUG(dbgs() << "IneqGraph: findShortestPathAt: " << *F << " ==> " << *T << " at " << BB->getName() << "\n");
  Value *SigmaOrF = VS->getSigmaForAt(F, BB);
  Value *SigmaOrT = VS->getSigmaForAt(T, BB);
  APInt Shortest = findShortestPath(SigmaOrF, SigmaOrT);
  APInt Other = findShortestPath(F, SigmaOrT);
  return Other.slt(Shortest) ? Other : Shortest;
}

// addMayEdge
void IneqGraph::addMayEdge(Value *F, Value *T, int64_t Weight) {
  addMayEdge(F, T, APInt(64, Weight, true));
}

// addMustEdge
// Adds the edge F -> T with weight Weight.
void IneqGraph::addMustEdge(Value *F, Value *T, APInt Weight) {
  DEBUG(dbgs() << "IneqGraph: addMustEdge: " << *F << " == " << Weight
                << " ==> " << *T << "\n");
  if (isa<ConstantInt>(T)) {
    // Make the edge to the alfa node a must edge.
    // F <= T + W ==> F == upper(T) + W ==> :alfa:
    GraphNode *FN = getOrCreateNode(F);
    FN->addMustEdgeTo(AlfaNode, RA->getRange(F).getUpper());
  } else {
    GraphNode *FN = getOrCreateNode(F);
    GraphNode *TN = getOrCreateNode(T);
    FN->addMustEdgeTo(TN, Weight);
  }
}

// adMustEdge
void IneqGraph::addMustEdge(Value *F, Value *T, int64_t Weight) {
  addMustEdge(F, T, APInt(64, Weight, true));
}

// addEdgesFor
// Creates a node for I and inserts edges from the created node to the
// appropriate node of other values.
void IneqGraph::addEdgesFor(Instruction *I) {
  if (I->getType()->isPointerTy())
    return;

  Range RI = RA->getRange(I);
  if (!RI.getLower().isMinSignedValue())
    addMayEdge(AlfaConst, I, -RI.getLower().getSExtValue());
  if (!RI.getUpper().isMaxSignedValue())
    addMayEdge(I, AlfaConst, RI.getUpper().getSExtValue());

  // TODO: Handle multiplication, remainder and division instructions.
  switch (I->getOpcode()) {
    case Instruction::SExt:
    case Instruction::ZExt:
    case Instruction::Trunc:
    case Instruction::BitCast:
      addMayEdge(I, I->getOperand(0), 0);
      addMayEdge(I->getOperand(0), I, 0);
      break;
    case Instruction::Add:
      // a = b + c
      // ==> a <= b + sup(c)
      // ==> a <= c + sup(b)
      // ==> b <= a - inf(c)
      // ==> c <= a - inf(b)
      {
        Value *A = I->getOperand(0);
        Value *B = I->getOperand(1);
        Range AR = RA->getRange(A);
        Range BR = RA->getRange(B);
        if (!isa<ConstantInt>(B) && !AR.getUpper().isMaxSignedValue())
          addMayEdge(I, B, AR.getUpper());
        if (!isa<ConstantInt>(A) && !BR.getUpper().isMaxSignedValue())
          addMayEdge(I, A, BR.getUpper());
        if (!isa<ConstantInt>(A) && !BR.getLower().isMinSignedValue())
          addMayEdge(A, I, -BR.getUpper());
        if (!isa<ConstantInt>(B) && !AR.getLower().isMinSignedValue())
          addMayEdge(B, I, -AR.getUpper());
        break;
      }
    case Instruction::Sub:
      // a = b - c
      // ==> a <= b - inf(c)
      {
        Value *A = I->getOperand(0);
        Value *B = I->getOperand(1);
        Range AR = RA->getRange(A);
        Range BR = RA->getRange(B);
        if (!isa<ConstantInt>(A) && !BR.getLower().isMinSignedValue())
          addMayEdge(I, A, -BR.getLower());
        break;
      }
    case Instruction::Br:
      // if (a > b) {
      //   a1 = sigma(a)
      //   b1 = sigma(b)
      {      
        BranchInst *BI = cast<BranchInst>(I);
        ICmpInst *Cmp = dyn_cast<ICmpInst>(I->getOperand(0));
        if (!Cmp)
          break;
        Value *L = Cmp->getOperand(0);
        DEBUG(dbgs() << "IneqGraph: L: " << *L << "\n");
        Value *R = Cmp->getOperand(1);
        DEBUG(dbgs() << "IneqGraph: R: " << *R << "\n");
        Value *LSigma = VS->findSigma(L, BI->getSuccessor(0), BI->getParent());
        DEBUG(dbgs() << "IneqGraph: LSigma: " << *LSigma << "\n");
        Value *RSigma = VS->findSigma(R, BI->getSuccessor(0), BI->getParent());
        DEBUG(dbgs() << "IneqGraph: RSigma: " << *RSigma << "\n");
        Value *LSExtSigma = VS->findSExtSigma(L, BI->getSuccessor(0), BI->getParent());
        DEBUG(dbgs() << "IneqGraph: LSExtSigma: " << *LSExtSigma << "\n");
        Value *RSExtSigma = VS->findSExtSigma(R, BI->getSuccessor(0), BI->getParent());
        DEBUG(dbgs() << "IneqGraph: RSExtSigma: " << *RSExtSigma << "\n");
        switch (Cmp->getPredicate()) {
          case ICmpInst::ICMP_SLT:
            DEBUG(dbgs() << "IneqGraph: SLT:\n");
            if (!isa<ConstantInt>(R) && LSigma) {
              if (RSigma)
                addMayEdge(LSigma, RSigma, -1);
              if (RSExtSigma)
                addMayEdge(LSigma, RSExtSigma, -1);
            }
            if (!isa<ConstantInt>(R) && LSExtSigma && LSExtSigma != LSigma) {
              if (RSigma)
                addMayEdge(LSExtSigma, RSigma, -1);
              if (RSExtSigma)
                addMayEdge(LSExtSigma, RSExtSigma, -1);
            }
            break;
          case ICmpInst::ICMP_SLE:
            DEBUG(dbgs() << "IneqGraph: SLE:\n");
            if (!isa<ConstantInt>(R) && LSigma && RSigma)
              addMayEdge(LSigma, RSigma, 0);
            if (!isa<ConstantInt>(R) && (LSExtSigma != LSigma || RSExtSigma != RSigma))
              addMayEdge(LSExtSigma, RSExtSigma, 0);
            break;
          default:
            break;
        }
        break;
      }
    case Instruction::PHI:
      {
        PHINode *Phi = cast<PHINode>(I);
        for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx) {
          addMustEdge(Phi, Phi->getIncomingValue(Idx), 0);
        }
        break;
      }
    case Instruction::Call:
      {
        CallInst *CI = cast<CallInst>(I);
        if (Function *F = CI->getCalledFunction()) {
          unsigned Idx = 0;
          for (Function::arg_iterator AI = F->arg_begin(), AE = F->arg_end();
               AI != AE; ++AI, ++Idx) {
            addMustEdge(&(*AI), CI->getArgOperand(Idx), 0);
          }
        }
        break;
      }
  }
}

void IneqGraph::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<InterProceduralRACousot>();
  AU.addRequired<vSSA>();
  AU.setPreservesAll();
}

void IneqGraph::print(raw_ostream &OS, const Module*) const {
  IneqGraph &IG = *const_cast<IneqGraph*>(this);

  OS << "digraph ineq_graph {\n";
  OS << "  label = \"Inequality graph for module\";\n";
  OS << "  node [shape = record,fontname = \"Times-Roman\", fontsize = 14];\n";

  for (NodeMapTy::iterator NI = IG.begin(), NE = IG.end(); NI != NE;
       ++NI)
    NI->second->print(OS);

  OS << "    label = \"Function: ALL\";\n";
  OS << "    graph[style = dotted];\n";
  OS << "}";
}

void IneqGraph::dumpToFile(const char *File, Module &M) {
  std::string ErrorMsg;
  raw_fd_ostream file(File, ErrorMsg);

  dbgs() << "Writing inequality graph to file " << File << "...\n";
  print(file, &M);

  file.close();
}

bool IneqGraph::runOnModule(Module &M) {
  C = &M.getContext();
  RA = (RangeAnalysis*)&getAnalysis<InterProceduralRACousot>();

  AlfaConst = ConstantInt::get(Type::getInt64Ty(*C), 0, true);
  AlfaNode = getOrCreateNode(AlfaConst);

  for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
    if (F->isDeclaration())
      continue;
    VS = &getAnalysis<vSSA>(*F);
    for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
      for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I)
        addEdgesFor(&(*I));
  }

  //if (isCurrentDebugType(DEBUG_TYPE))
  //  dumpToFile("ineq_graph.module.dot", M);

  return false;
}

char IneqGraph::ID = 0;
static RegisterPass<IneqGraph> X("ineq-graph", "Inequality graph");

