#ifndef NETLEVEL_H
#define NETLEVEL_H

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <utility>

#include <boost/graph/graphviz.hpp>
#include <boost/graph/copy.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/DOTGraphTraits.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/PassAnalysisSupport.h"

#include "GraphStructure.h"
#include "FunctionInfo.h"

using namespace std;

namespace {

//bool map_comp (pair<string,int> lhs, pair<string,int> rhs) {
//    return lhs.first<rhs.first || (lhs.first==rhs.first && lhs.second<rhs.second);
//}

//bool(*mapComp_pt)(pair<string,int>, pair<string,int>) = map_comp;

class PairKey
{
  public:
    PairKey(std::string s, int i)
    {
      this->s = s;
      this->i = i;
    }
    std::string s;
    int i;
    bool operator<(const PairKey& k) const
    {
      int s_cmp = this->s.compare(k.s);
      if(s_cmp == 0)
      {
        return this->i < k.i;
      }
      return s_cmp < 0;
    }
};

struct ProgramStructure {
    map<string, FunctionInfo*> functionsMap; // Graph instances
    FunctionInfo* fullNetSendGraph;
    FunctionInfo* fullNetRecvGraph;
    vector<set<Vertex> > levelSetsSends;
    vector<set<Vertex> > levelSetsRecvs;

//    map< pair<string,int>, set<int>, bool(*)(pair<string,int>, pair<string,int>) > nodeLevelSetsSends(mapComp_pt);
//    map< pair<string,int>, set<int>, bool(*)(pair<string,int>, pair<string,int>) > nodeLevelSetsRecvs(mapComp_pt);
    map< PairKey, set<int> > nodeLevelSetsSends;
    map< PairKey, set<int> > nodeLevelSetsRecvs;

    map<string, Vertex> nodesMap;

    ProgramStructure() {
        fullNetSendGraph = new FunctionInfo("full_ns");
        fullNetRecvGraph = new FunctionInfo("full_nr");
    }

    ~ProgramStructure() {
        for( map<string, FunctionInfo *>::iterator ir = functionsMap.begin(); ir != functionsMap.end(); ++ir ) {
            delete ir->second;
        }
    }
};
}

enum ProgramNumber {
    ONE = 1, TWO = 2
};

enum NetFunctionType {
    SEND = 1, RECV = 2
};

template<typename GraphType>
class NetLevel: public llvm::ModulePass {
public:
    static char ID; // Pass identification, replacement for typeid

    NetLevel();

    virtual bool runOnModule(llvm::Module &M);

    map< PairKey, set<int> >* getNodesData(ProgramNumber programNumber, NetFunctionType netFunctionType) {
        map< PairKey, set<int> > *nodesMap;
        vector<set<Vertex> > levelSets;
        FunctionInfo *functionInfo;
        ProgramStructure *program = NULL;
        switch (programNumber) {
        case ONE:
            program = &program1;
            break;
        case TWO:
            program = &program2;
            break;
        }

        switch(netFunctionType) {
        case SEND:
            levelSets = program->levelSetsSends;
            functionInfo = program->fullNetSendGraph;
            nodesMap = &program->nodeLevelSetsSends;
            break;
        case RECV:
            levelSets = program->levelSetsRecvs;
            functionInfo = program->fullNetRecvGraph;
            nodesMap = &program->nodeLevelSetsRecvs;
            break;
        }

        unsigned int level = 0;
        for (vector<set<Vertex> >::iterator it = levelSets.begin();
             it != levelSets.end(); ++it) {
            for (set<Vertex>::iterator vertIt = levelSets[level].begin();
                 vertIt != levelSets[level].end(); ++vertIt) {
                string codeName = functionInfo->codeNames[*vertIt];
                int codeLine = functionInfo->codeLines[*vertIt];
                string key = codeName + ':' + boost::lexical_cast<string>(codeLine);
                if(nodesMap->count(PairKey(codeName, codeLine)) == 0) {
                    nodesMap->insert(std::make_pair(PairKey(codeName, codeLine), set<int>()));
                }
                set<int> *levels = &(nodesMap->at(PairKey(codeName, codeLine)));
                levels->insert(level);
            }
            ++level;
        }

        return nodesMap;
    }

    set<int> getNodeLevels(ProgramNumber programNumber, NetFunctionType netFunctionType, string fileName, int line) {
        map< PairKey, set<int> > *nodesMap;
        ProgramStructure *program = NULL;
        switch (programNumber) {
        case ONE:
            program = &program1;
            break;
        case TWO:
            program = &program2;
            break;
        }

        switch(netFunctionType) {
        case SEND:
            nodesMap = &program->nodeLevelSetsSends;
            break;
        case RECV:
            nodesMap = &program->nodeLevelSetsRecvs;
            break;
        }


        if(nodesMap->count(PairKey(fileName, line)) != 0) {
            return nodesMap->at(PairKey(fileName, line));
        } else {
            return set<int>();
        }
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const;
    void createLevels();
private:
    ProgramStructure program1;
    ProgramStructure program2;

    llvm::StringRef outputDir;
    llvm::StringRef program1EntryFunction;
    llvm::StringRef program2EntryFunction;

    typedef llvm::GraphTraits<GraphType> GTraits;
    typedef typename GTraits::nodes_iterator node_iterator;
    typedef typename GTraits::ChildIteratorType child_iterator;

    void getModules(llvm::Module &M);

    void getInstructions(ProgramStructure *program, llvm::BasicBlock &B, list<Vertex> u1list,
                         Vertex &retVert, const string &functionName);


    void replaceFunctionVertices(FunctionInfo *functionInfo);

    void writeCFGsToFiles();

    void returnCodeInfo(llvm::MDNode* N, VertexExtraData **extraData);

    ProgramStructure* detectProgram(const llvm::StringRef& functionName);

};

#endif // NETLEVEL_H
