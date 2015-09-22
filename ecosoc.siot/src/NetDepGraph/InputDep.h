#ifndef __INPUTDEP_H__
#define __INPUTDEP_H__

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/User.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/DebugInfo.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ilist.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/IR/Metadata.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/IR/Function.h"
//#include "DepGraph.h"
#include "llvm/ADT/StringRef.h"
#include<set>

using namespace llvm;
class Input{
	Value* value;
	int type; //1 - Net , 0 - Others
	int programId;
	std::string instName;
	public:
		Input(Value *value,int type, int programId){
			this->value = value;
			this->type = type;
			this->programId = programId;
		}
	int getProgramId(){
            return this->programId;
        }
	int getType(){
            return this->type;
        }
	Value * getValue(){
            return this->value;
        }
	bool isNet(){
		if(type==1) return true;
		else return false;
	}
	std::string getName(){
		std::string aux_name = value->getName().str();
		DEBUG(errs()<<"Name["<<aux_name<<"] instName ["<<instName<<"]\n");
		if(aux_name.empty()) aux_name = instName;
		if(type==1) {
			return "NetInput " + aux_name;
		}else {
			return "Input " + aux_name; 
		}
	}
	std::string getLabel(){
		return this->getName();
	}
	void setInstName(std::string instName){
		this->instName = instName;
	}
	
};

class InputDep : public ModulePass {
	private:
		std::set<Input*> inputDepValues;
        	std::set<Input*> netInputDepValues;
		std::set<Input*> inputDepValuesWithNetSources;
		std::set<Input*> inputDepValues2;
        std::set<Input*> netInputDepValues2;
		std::set<Input*> inputDepValues2WithNetSources;
		std::map<Input*, int> lineNo;
		bool runOnModule(Module &M);
	public:
		static char ID; // Pass identification, replacement for typeid.
		InputDep();
		void getAnalysisUsage(AnalysisUsage &AU) const;
		bool isInputDependent(Value* V);

        //TODO: choose the program target. This version works with only 2 programs
        std::set<Input*> getInputDepValues(int programId);
        std::set<Input*> getNetInputDepValues(int programId);
        std::set<Input*> getInputDepValuesWithNetSources(int programId);

        int getLineNo(Value*);
		void printer();
        void processInput(Function *Callee, CallInst *CI, std::set<Input*> *inputDeps, int programId, bool skipNetSources );
        //void processInput(Function *Callee, Value *v, std::set<Value*> *inputDeps,  bool skipNetSources );
};

#endif
