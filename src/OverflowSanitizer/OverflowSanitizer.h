/*
 * OverflowSanitizer.h
 *
 *
 *      Author: izabela
 *      Thanks to: raphael
 */

#ifndef OVERFLOWSANITIZER_H_
#define OVERFLOWSANITIZER_H_


#include "llvm/IR/Attributes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/DebugInfo.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/IR/Operator.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Local.h"

#include <vector>
#include <set>
#include <list>
#include <stdio.h>

#include "../../Analysis/DepGraph/InputValues.h"
#include "../../Analysis/DepGraph/DepGraph.h"
#include "../uSSA/uSSA.h"

using namespace llvm;

namespace llvm{

	typedef enum { OvUnknown, OvCanHappen, OvWillHappen, OvWillNotHappen } OvfPrediction;

	struct OverflowSanitizer : public ModulePass {
	private:
		Module* module;
		std::set<Instruction*> valuesToSafe;
		llvm::DenseMap<Function*, BasicBlock*> abortBlocks;
        llvm::LLVMContext* context;
        Constant* constZero;
        Value* GVstderr, *FPrintF, *overflowMessagePtr, *truncErrorMessagePtr, *overflowMessagePtr2, *truncErrorMessagePtr2;
        // Pointer to abort function
        Function *AbortF;
        std::map<std::string,Constant*> SourceFiles;

        void markAsNotOriginal(Instruction& inst);
        bool isNotOriginal(Instruction& inst);
        static bool isValidInst(Instruction *I);
		BasicBlock* newOverflowOccurrenceBlock(Instruction* I, BasicBlock* NextBlock, Value *messagePtr);
		void insertInstrumentation(Instruction* I, BasicBlock* AbortBB, OvfPrediction Pred);
		Constant* getSourceFile(Instruction* I);
		Constant* getLineNumber(Instruction* I);
		Instruction* getNextInstruction(Instruction& i);
		Constant* strToLLVMConstant(std::string s);
		int countInstructions();
		int countOverflowableInsts();
		void insertGlobalDeclarations();

	public:
		static char ID;
		OverflowSanitizer() : ModulePass(ID), module(NULL), context(NULL),
							  constZero(NULL), GVstderr(NULL), FPrintF(NULL), overflowMessagePtr(NULL),
							  truncErrorMessagePtr(NULL), overflowMessagePtr2(NULL),
							  truncErrorMessagePtr2(NULL), AbortF(NULL){};

		virtual bool runOnModule(Module &M);

		virtual void getAnalysisUsage(AnalysisUsage &AU) const;

	};
}
#endif /* OVERFLOWSANITIZER_H_ */
