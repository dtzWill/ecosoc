#ifndef LLVM_TRANSFORMS_INEQUALITYGRAPH_INEQUALITYGRAPH_H_
#define LLVM_TRANSFORMS_INEQUALITYGRAPH_INEQUALITYGRAPH_H_

#include "llvm/Pass.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "RangeAnalysis.h"

namespace llvm {

class GraphNode;

class GraphEdge : public ilist_node<GraphEdge> {
public:
  enum EdgeType { NONE, MAY, MUST, BYTE, INDEX };

  GraphEdge();
  GraphEdge(GraphNode *From, GraphNode *To, EdgeType ETy, APInt Weight);

  void print(raw_ostream &OS);

  GraphNode *getToEdge() {
    return To;
  }

  APInt getWeight() {
    return Weight;
  }

  void setEdgeType(EdgeType ET) {
    ETy = ET;
  }

private:
  EdgeType ETy;
  APInt Weight;
  GraphNode *From, *To;
};

class GraphNode {
public:
  typedef iplist<GraphEdge>::iterator may_iterator; 
  typedef iplist<GraphEdge>::iterator must_iterator; 

  may_iterator may_begin() { return MayEdges.begin(); } 
  may_iterator may_end() { return MayEdges.end(); } 

  must_iterator must_begin() { return MustEdges.begin(); } 
  must_iterator must_end() { return MustEdges.end(); } 

  Value *getValue() { return V; }

  GraphNode(Value *V) : V(V) { }

  /* void getReachableNodes(SetVector<GraphNode*> &Mark);
  void intersect(SetVector<GraphNode*> &Mark, SetVector<GraphNode*> &Other);
  void removeMustEdges(); */
  //APInt findShortestPath(GraphNode *Dest);
  APInt findShortestPath(GraphNode *Dest, SetVector<GraphNode*> Visited = SetVector<GraphNode*>());

  void addMayEdgeTo(GraphNode *To, APInt Weight) {
    GraphEdge *Edge = new GraphEdge(this, To, GraphEdge::MAY, Weight);
    MayEdges.push_back(Edge);
  }

  void addMustEdgeTo(GraphNode *To, APInt Weight) {
    GraphEdge *Edge = new GraphEdge(this, To, GraphEdge::MUST, Weight);
    MustEdges.push_back(Edge);
  }

  void print(raw_ostream &OS) {
    for (may_iterator MI = may_begin(), ME = may_end(); MI != ME; ++MI)
      MI->print(OS);

    for (must_iterator MI = must_begin(), ME = must_end(); MI != ME; ++MI)
      MI->print(OS);
  }

private:
  Value *V;
  iplist<GraphEdge> MayEdges;
  iplist<GraphEdge> MustEdges;
};

class IneqGraph : public ModulePass {
public:
  typedef DenseMap<Value*, GraphNode*> NodeMapTy;

  static char ID;
  IneqGraph()
    : ModulePass(ID) { }

  NodeMapTy::iterator begin() { return NodeMap.begin(); }
  NodeMapTy::iterator end() { return NodeMap.end(); }

  APInt getUpper(Value *V) {
    if (ConstantInt *CI = dyn_cast<ConstantInt>(V))
      return CI->getValue();
    return RA->getRange(V).getUpper();
  }

  APInt getLower(Value *V) {
    if (ConstantInt *CI = dyn_cast<ConstantInt>(V))
      return CI->getValue();
    return RA->getRange(V).getLower();
  }

  APInt getUpperAt(Value *V, BasicBlock *BB) {
    if (ConstantInt *CI = dyn_cast<ConstantInt>(V))
      return CI->getValue();
    Value *SigmaOrSelf = V;
    if (BB->getSinglePredecessor())
      SigmaOrSelf = VS->getSigmaForAt(V, BB); //, BB->getSinglePredecessor());
    return getUpper(SigmaOrSelf);
  }

  APInt getLowerAt(Value *V, BasicBlock *BB) {
    DEBUG(dbgs() << "getLowerAt: V: " << *V << "\n");
    if (ConstantInt *CI = dyn_cast<ConstantInt>(V))
      return CI->getValue();
    DEBUG(dbgs() << "getLowerAt: V: " << *V << "\n");
    Value *SigmaOrSelf = VS->getSigmaForAt(V, BB); //, BB->getSinglePredecessor());
    DEBUG(dbgs() << "SigmaOrSelf: " << *SigmaOrSelf << "\n");
    return getLower(SigmaOrSelf);
  }

  void addMayEdge(Value *F, Value *T, APInt Weight);
  void addMayEdge(Value *F, Value *T, int64_t Weight);

  void addMustEdge(Value *F, Value *T, APInt Weight);
  void addMustEdge(Value *F, Value *T, int64_t Weight);

  APInt findShortestPath(Value *F, Value *T);
  APInt findShortestPathAt(Value *F, Value *T, BasicBlock *BB);

  void dumpToFile(const char *File, Module &M);

private:
  GraphNode *getOrCreateNode(Value *V);
  GraphNode *getNode(Value *V);

  void addEdgesFor(Instruction *I);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual void print(raw_ostream &OS, const Module*) const;
  virtual bool runOnModule(Module &M);

  ConstantInt *AlfaConst;
  GraphNode *AlfaNode;

  LLVMContext *C;

  vSSA *VS;
  RangeAnalysis *RA;

  // Maps a value to it's node in the graph.
  NodeMapTy NodeMap;
};

}

#endif

