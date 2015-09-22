//===- Hello.cpp - Example code from "Writing an LLVM Pass" ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements two versions of the LLVM "Hello World" pass described
// in docs/WritingAnLLVMPass.html
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "merge"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include <string>
using namespace llvm;

static cl::opt<std::string> Flag("FLG", cl::desc("Specify function's flag"), cl::value_desc("flag"));
static cl::opt<std::string> SF("SF0", cl::desc("Specify Send function name"), cl::value_desc("sendfunc"));
static cl::opt<std::string> SF2("SF1", cl::desc("Specify Send function name"), cl::value_desc("sendfunc"));
static cl::opt<std::string> SF3("SF2", cl::desc("Specify Send function name"), cl::value_desc("sendfunc"));
static cl::opt<std::string> SF4("SF3", cl::desc("Specify Send function name"), cl::value_desc("sendfunc"));
static cl::opt<std::string> RF("RF0", cl::desc("Specify Receive function name"), cl::value_desc("recvfunc"));
static cl::opt<std::string> RF2("RF1", cl::desc("Specify Receive function name"), cl::value_desc("recvfunc"));
static cl::opt<std::string> RF3("RF2", cl::desc("Specify Receive function name"), cl::value_desc("recvfunc"));
static cl::opt<std::string> RF4("RF3", cl::desc("Specify Receive function name"), cl::value_desc("recvfunc"));


STATISTIC(FuncCounter, "Counts number of functions");

namespace {
	
   // Merge- 
  struct Merge: public ModulePass {
    static char ID; // Pass identification, replacement for typeid
    Merge() : ModulePass(ID) {}
    
    virtual bool runOnModule(Module &M) {
		
		StringRef Flg = Flag;
		
		StringRef SendFunc = SF;
		StringRef RecvFunc = RF;
		
		StringRef SendFunc2, SendFunc3, SendFunc4,
		RecvFunc2, RecvFunc3, RecvFunc4;
		
		if(SF2.empty())
		SendFunc2 = SF;
		else
		SendFunc2 = SF2;
		if(SF3.empty())
		SendFunc3 = SF;
		else
		SendFunc3 = SF3;
		if(SF4.empty())
		SendFunc4 = SF;
		else
		SendFunc4 = SF4;
		
				
		if(RF2.empty())
		RecvFunc2 = RF;
		else
		RecvFunc2 = RF2;
		if(RF3.empty())
		RecvFunc3 = RF;
		else
		RecvFunc3 = RF3;
		if(RF4.empty())
		RecvFunc4 = RF;
		else
		RecvFunc4 = RF4;
		
       // Rename all global variables
       if(Flg.endswith("3")){
		for (Module::global_iterator GI = M.global_begin(), GE = M.global_end();
            GI != GE; ++GI) {
			StringRef Name = GI->getName();
			if(!Name.startswith("llvm.dbg.declare"))
				GI->setName(Twine("ttt_")+Name);
			}
		}
		
		 // Rename all functions
		for (Module::iterator FI = M.begin(), FE = M.end();
            FI != FE; ++FI){ 
				
		std::string Name = FI->getName();
		std::string Name2;
		//StringRef Name2 = FI->getName();
		Name2 = Name+"[";
		
		if(Flg.equals("0")){
			if(Name.find("recv")  != std::string::npos || Name.find("send") != std::string::npos
			|| Name.find("read") != std::string::npos || Name.find("write") != std::string::npos){
			errs() << Name << "(";
			runOnFunction(*FI);
			}
		}
		if(Flg.startswith("1")){
			if(SendFunc.startswith(Name2)){
			 FI->setName(Twine("sss_"+SendFunc));
			}
			if(SendFunc2.startswith(Name2)){
			 FI->setName(Twine("sss_"+SendFunc2));
			}
			if(SendFunc3.startswith(Name2)){
			 FI->setName(Twine("sss_"+SendFunc3));
			}
			if(SendFunc4.startswith(Name2)){
			 FI->setName(Twine("sss_"+SendFunc4));
			}		
				 
			 if(RecvFunc.startswith(Name2)){
		//DEBUG(errs()<<"Merge - RecvFunc"<<RecvFunc<<"\n");
                FI->setName(Twine("rrr_"+RecvFunc));
                // F is a pointer to a Function instance
                for (inst_iterator I = inst_begin(FI), E = inst_end(FI); I != E; ++I)
                    errs() << *I << "\n";
			}
			 if(RecvFunc2.startswith(Name2)){
			 FI->setName(Twine("rrr_"+RecvFunc2));
			}
			 if(RecvFunc3.startswith(Name2)){
			 FI->setName(Twine("rrr_"+RecvFunc3));
			}
			 if(RecvFunc4.startswith(Name2)){
			 FI->setName(Twine("rrr_"+RecvFunc4));
			}
		}
		if(Flg.endswith("3")){
		StringRef NameFunc = FI->getName();
		 if(!NameFunc.startswith("llvm.dbg"))
			 FI->setName("ttt_"+NameFunc);

	 }
	 }
		if(Flg.endswith("3")){
		for (Module::iterator FI = M.begin(), FE = M.end();
		            FI != FE; ++FI){
			for(Function::iterator BB = FI->begin(),
			                E = FI->end(); BB!=E; ++BB){

			        for(BasicBlock::iterator I = BB->begin(),
			                E = BB->end(); I != E; ++I){
			        	if(CallInst* CI = dyn_cast<CallInst>(I)){
			        		if(Function *F = CI->getCalledFunction())
			        		{
			        			if(F->getName().startswith("llvm.dbg.declare")){

									Value* referredValue = cast<MDNode>(CI->getOperand(0))->getOperand(0);
									if(referredValue != NULL)
									{
										StringRef NameFunc = referredValue->getName();
										referredValue->setName("ttt_" + NameFunc);
									}

//			        				errs()<<referredValue->getName()<<"\n";
			        			}
			        		}
			        	}

			        }
			        }
		}
    }
		
		return false;		
	}

    virtual bool runOnFunction(Function &F) {
      ++FuncCounter;
      
	for(Function::arg_iterator AI = F.arg_begin(), AE = F.arg_end();
		AI != AE; ++AI){
			
			StringRef Name = AI->getName();						
			errs() << Name;			
			errs() << " ";				
		}	
	
		errs() << ") \n";

      return false;
    }
    
    
    
  };
}

char Merge::ID = 0;
static RegisterPass<Merge> X("Merge", "Merge BC Pass");


