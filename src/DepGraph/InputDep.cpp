#define DEBUG_TYPE "inputdep"
#include "InputDep.h"
STATISTIC(NumInputValues, "The number of input values");

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
	NumInputValues = 0;
	bool inserted;
	Function* main = M.getFunction("main");
	if (main) {
		MDNode *mdn = main->begin()->begin()->getMetadata("dbg");
		for (Function::arg_iterator Arg = main->arg_begin(), aEnd =
				main->arg_end(); Arg != aEnd; Arg++) {
			inputDepValues.insert(Arg);
			NumInputValues++;
			if (mdn) {
				DILocation Loc(mdn);
				unsigned Line = Loc.getLineNumber();
				lineNo[Arg] = Line-1; //educated guess (can only get line numbers from insts, suppose main is declared one line before 1st inst
			}
		}


	}
	for (Module::iterator F = M.begin(), eM = M.end(); F != eM; ++F) {
		for (Function::iterator BB = F->begin(), e = F->end(); BB != e; ++BB) {
			for (BasicBlock::iterator I = BB->begin(), ee = BB->end(); I != ee; ++I) {
				if (CallInst *CI = dyn_cast<CallInst>(I)) {
					Function *Callee = CI->getCalledFunction();
					if (Callee) {
						Value* V;
						inserted = false;
						StringRef Name = Callee->getName();
						if (Name.equals("main")) {
							errs() << "main\n";
							V = CI->getArgOperand(1); //char* argv[]
							inputDepValues.insert(V);
							inserted = true;
							//errs() << "Input    " << *V << "\n";
						}
						if (Name.equals("__isoc99_scanf") || Name.equals(
								"scanf")) {
							for (unsigned i = 1, eee = CI->getNumArgOperands(); i
									!= eee; ++i) { // skip format string (i=1)
								V = CI->getArgOperand(i);
								if (V->getType()->isPointerTy()) {
									inputDepValues.insert(V);
									inserted = true;
									//errs() << "Input    " << *V << "\n";
								}
							}
						} else if (Name.equals("__isoc99_fscanf")
								|| Name.equals("fscanf")) {
							for (unsigned i = 2, eee = CI->getNumArgOperands(); i
									!= eee; ++i) { // skip file pointer and format string (i=1)
								V = CI->getArgOperand(i);
								if (V->getType()->isPointerTy()) {
									inputDepValues.insert(V);
									inserted = true;
									//errs() << "Input    " << *V << "\n";
								}
							}
						} else if ((Name.equals("gets") || Name.equals("fgets")
								|| Name.equals("fread"))
								|| Name.equals("getwd")
								|| Name.equals("getcwd")) {
							V = CI->getArgOperand(0); //the first argument receives the input for these functions
							if (V->getType()->isPointerTy()) {
								inputDepValues.insert(V);
								inserted = true;
								//errs() << "Input    " << *V << "\n";
							}
						} else if ((Name.equals("fgetc") || Name.equals("getc")
								|| Name.equals("getchar"))) {
							inputDepValues.insert(CI);
							inserted = true;
							//errs() << "Input    " << *CI << "\n";
						} else if (Name.equals("recv")
								|| Name.equals("recvmsg")
								|| Name.equals("read")) {
							Value* V = CI->getArgOperand(1);
							if (V->getType()->isPointerTy()) {
								inputDepValues.insert(V);
								inserted = true;
								//errs() << "Input    " << *V << "\n";
							}
						} else if (Name.equals("recvfrom")) {
							V = CI->getArgOperand(1);
							if (V->getType()->isPointerTy()) {
								inputDepValues.insert(V);
								inserted = true;
								//errs() << "Input    " << *V << "\n";
							}
							V = CI->getArgOperand(4);
							if (V->getType()->isPointerTy()) {
								inputDepValues.insert(V);
								inserted = true;
								//errs() << "Input    " << *V << "\n";
							}
						}
						if (inserted) {
							if (MDNode *mdn = I->getMetadata("dbg")) {
								NumInputValues++;
								DILocation Loc(mdn);
								unsigned Line = Loc.getLineNumber();
								lineNo[V] = Line;
							}
						}
					}
				}
			}
		}
	}
	DEBUG(printer());
	return false;
}

void InputDep::printer() {
	errs() << "===Input dependant values:====\n";
	for (std::set<Value*>::iterator i = inputDepValues.begin(), e =
			inputDepValues.end(); i != e; ++i) {
		errs() << **i << "\n";
	}
	errs() << "==============================\n";
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
