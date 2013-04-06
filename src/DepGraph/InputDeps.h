/*
 * InputDeps.h
 *
 *  Created on: 05/04/2013
 *      Author: raphael
 */

#ifndef INPUTDEPS_H_
#define INPUTDEPS_H_

#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Support/raw_ostream.h"
#include <set>
#include <string>

using namespace std;

namespace llvm {


        class InputDeps : public ModulePass {
        private:
			llvm::Module* module;

			std::set<Function*> whiteList;
			std::set<Value*> inputDepValues;

            void initializeWhiteList();
            void insertInWhiteList(Function* F);
            void insertInInputDepValues(Value* V);

            bool isMarkedCallInst(CallInst* CI);

            void collectMainArguments();
        public:
			static char ID; // Pass identification, replacement for typeid.
			InputDeps() : ModulePass(ID), module(NULL){}

			void getAnalysisUsage(AnalysisUsage &AU) const;
			bool runOnModule(Module& M);

			bool isInputDependent(Value* V);

        };

}



#endif /* INPUTDEPS_H_ */
