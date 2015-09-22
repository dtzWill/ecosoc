#ifndef __VUL_ARRAYS_H__
#define __VUL_ARRAYS_H__

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/User.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Metadata.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/Function.h"
#include "llvm/DebugInfo.h"
#include "DepGraph.h"
#include "llvm/ADT/StringRef.h"
#include "InputValues.h"
#include "InputDep.h"
#include "AddStore.h"
#include<set>
#include<utility>
#include <ctime>

using namespace llvm;

class Program {
	int id;
    DenseMap<GraphNode*, std::map<GraphNode*, std::vector<GraphNode*> > > NetReadArrays;
  	DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > Deps; // to avoid repeated computation
    DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > CrossDeps; // to avoid repeated computation
    std::set<Input*> inputDepValues, crossInputDepValues, netInputDepValues;
	public: 
		Program(int Id,  std::set<Input*> InputDepValues){
			id = Id;
			inputDepValues = InputDepValues;
		}

        int getId(){
            return this->id;
        }
		void setId(int Id){
			id = Id;
		}
		std::set<Input*> getInputDepValues(){
			return inputDepValues;
		}
		std::set<Input*> getCrossInputDepValues(){
			return crossInputDepValues;
		}
		void setCrossInputDepValues(std::set<Input*> crossInputDeps ){
			crossInputDepValues = crossInputDeps;
		}
        std::set<Input*> getNetInputDepValues(){
            return netInputDepValues;
        }
        void setNetInputDepValues(std::set<Input*> netInputDeps ){
            crossInputDepValues = netInputDeps;
        }

  		void setDeps(bool skipNetworkNodes, GraphNode* N, DenseMap<const Value*, std::vector<GraphNode*> > deps){
			if (skipNetworkNodes) {
				Deps[N] = deps;
			} else {
				CrossDeps[N] = deps;
			}
		}

  		DenseMap<const Value*, std::vector<GraphNode*> >* getDeps(bool skipNetworkNodes, GraphNode* N){
        		if (skipNetworkNodes){
				if(Deps.count(N)) {
					DEBUG(errs() << "Return Deps.count(N)\n");
					return &Deps[N];
				} else  return NULL;
			} else if( CrossDeps.count(N)){
				DEBUG(errs() << "Return CrossDeps.count(N)\n");
				return &CrossDeps[N];
			} else return NULL;
		}

        void setNetReadDeps( GraphNode* N, std::map<GraphNode*, std::vector<GraphNode*> > deps){
            NetReadArrays[N]=deps;
        }
         DenseMap<GraphNode*, std::map<GraphNode*, std::vector<GraphNode*> > > getNetReadArrays(){
            return NetReadArrays;
        }
};

class NetVulArrays : public ModulePass {
	private:
		bool runOnModule(Module &M);
		void searchForArray(Value* V);
		void findVulLocals(const Value* V);
		bool structHasArray(StructType* ST);
        int getVul(std::set<MemNode *> memNodes1,std::set<MemNode*> inputDeps1,Graph* depGraph, std::vector< std::vector<GraphNode*> > &visitedNodesList, int *abc );
        void setInputNode(std::set<Input *> inputDeps, int  programI, std::set<GraphNode*> &INListD);
        //vulnerable node n: (input value v: path from v to n)
        std::set<GraphNode *> Arrays1;
        std::set<GraphNode *> Arrays2;

	DenseMap<std::pair<GraphNode*, GraphNode*>, std::vector<GraphNode*> > visitedNodesMap; //the first GraphNode is the source, the second GraphNode is the sink and the vector is the visited flow

        DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > VulArrays1;
        DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > VulArrays2;
        DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > CrossVulArrays1;
        DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > CrossVulArrays2;
        DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > NetReadArrays1;
        DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > NetReadArrays2;

		DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > Structs1;
		DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > Structs2;
		DenseMap<std::pair<GraphNode*, GraphNode*>, std::pair<unsigned, std::string> > debugInfo;
		Graph* depGraph;

        const Value* isValueInpDep(Value* V, int vProgramID, std::set<Value*> inputDepValues, int inputProgramID);

        DenseMap<const Value*, std::vector<GraphNode*> > getValueDeps(Value* V, int sinkProgramID, Program *program , bool skipNetworkNodes);
		void toDot(std::string name);
		void toDotFocusDFS(std::vector<std::vector<GraphNode*> > visitedNodesList,std::string fileName);
//		void printStats();
	public:
		static char ID;
		void getAnalysisUsage(AnalysisUsage &AU) const;
		NetVulArrays();
		void printArrays();
		void printStats();
		DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > getVulArrays();
		DenseMap<GraphNode*, DenseMap<const Value*, std::vector<GraphNode*> > > getVulStructs();

};
#endif
