#define DEBUG_TYPE "inputdep"
#include "InputDep.h"

InputDep::InputDep() :
	ModulePass(ID) {
}

//TODO: Verify signature


/*
 * Main args are always input
 * Functions currently considered as input functions:
 * scanf
 * fscanf
 * gets
 * fgets
 * fread
 * fgetc
 * getc
 * getchar
 * recv
 * recvmsg
 * read
 * recvfrom
 * fread
 */
bool InputDep::runOnModule(Module &M) {
	//	DEBUG (errs() << "Function " << F.getName() << "\n";);
	for (Module::iterator F = M.begin(), eM = M.end(); F != eM; ++F) {
		for (Function::iterator BB = F->begin(), e = F->end(); BB != e; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), ee = BB->end(); I != ee; ++I) {
				if (CallInst *CI = dyn_cast<CallInst>(I)) {
					Function *Callee = CI->getCalledFunction();
					if (Callee) {
						StringRef Name = Callee->getName();
						if (Name.equals("main")) {
							Value* V = CI->getArgOperand(1); //char* argv[]
							inputDepValues.insert(V);
							//errs() << "Input    " << *V << "\n";
						}
						if (Name.equals("__isoc99_scanf") || Name.equals(
								"scanf")) {
							for (unsigned i = 1, eee = CI->getNumArgOperands(); i
									!= eee; ++i) { // skip format string (i=1)
								Value* V = CI->getArgOperand(i);
								if (V->getType()->isPointerTy()) {
									inputDepValues.insert(V);
									//errs() << "Input    " << *V << "\n";
								}
							}
						} else if (Name.equals("__isoc99_fscanf")
								|| Name.equals("fscanf")) {
							for (unsigned i = 2, eee = CI->getNumArgOperands(); i
									!= eee; ++i) { // skip file pointer and format string (i=1)
								Value* V = CI->getArgOperand(i);
								if (V->getType()->isPointerTy()) {
									inputDepValues.insert(V);
									//errs() << "Input    " << *V << "\n";
								}
							}
						} else if ((Name.equals("gets") || Name.equals("fgets")
								|| Name.equals("fread"))
								|| Name.equals("getwd")
								|| Name.equals("getcwd")) {
							Value* V = CI->getArgOperand(0); //the first argument receives the input for these functions
							if (V->getType()->isPointerTy()) {
								inputDepValues.insert(V);
								//errs() << "Input    " << *V << "\n";
							}
						} else if ((Name.equals("fgetc") || Name.equals("getc")
								|| Name.equals("getchar"))) {
							inputDepValues.insert(CI);
							//errs() << "Input    " << *CI << "\n";
						} else if (Name.equals("recv")
								|| Name.equals("recvmsg")
								|| Name.equals("read")) {
							Value* V = CI->getArgOperand(1);
							if (V->getType()->isPointerTy()) {
								inputDepValues.insert(V);
								//errs() << "Input    " << *V << "\n";
							}
						} else if (Name.equals("recvfrom")) {
							Value* V = CI->getArgOperand(1);
							if (V->getType()->isPointerTy()) {
								inputDepValues.insert(V);
								//errs() << "Input    " << *V << "\n";
							}
							V = CI->getArgOperand(4);
							if (V->getType()->isPointerTy()) {
								inputDepValues.insert(V);
								//errs() << "Input    " << *V << "\n";
							}
						}
					}
				} //else
				//						errs() << "indirect call\n"; // Ponteiro pra função: verificar alias?
			}
		}
	}

	return false;
}

void InputDep::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.setPreservesAll();
}

std::set<Value*> InputDep::getInputDepValues() {
	return inputDepValues;
}

char InputDep::ID = 0;
static RegisterPass<InputDep> X("inputDep",
		"Input Dependency Pass: looks for values that are input-dependant");
