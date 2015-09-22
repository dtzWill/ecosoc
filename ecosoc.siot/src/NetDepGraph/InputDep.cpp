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
 * the recv, recvform and recvmsg ware removed from the NetDepGraph version
 */
void InputDep::processInput(Function *Callee, CallInst *CI,std::set<Input*> *inputDep, int programId, bool skipNetSources){
    Value* V;
    bool inserted;
    inserted = false;

    StringRef Name = Callee->getName();

    std::string::size_type    start_position = 0;
    std::string::size_type    end_position = 0;
    std::string               text = Callee->getName();
    std::string               found_text;

    start_position = text.find("[");
    if (start_position != std::string::npos)
    {
        ++start_position; // start after the double quotes.
        // look for end position;
        end_position = text.find("]");
        if (end_position != std::string::npos)
        {
            found_text = text.substr(start_position, end_position - start_position);
        }
    }

    //istringstream buffer(found_text);
    //int RecvFuncArg;
    //int RecvFuncArg2;
    //buffer >> RecvFuncArg;
    //RecvFuncArg2 = RecvFuncArg;

    int RecvFuncArg = std::atoi(found_text.c_str());
    int RecvFuncArg2 = RecvFuncArg;

    //errs() <<found_text<<"\n"<<text<<"\n"<<RecvFuncArg<<"\n";


    //	DEBUG(errs() << "processInput Callee Name["<<Name<<"\n");
    if (Name.equals("main") || Name.equals("ttt_main")){
        errs() << "main\n";
        V = CI->getArgOperand(1); //char* argv[]
        Input *i = new Input(V,0,programId);
	i->setInstName(Name);
        inputDep->insert(i);
        inserted = true;
       	DEBUG(errs() << "Input argv[" << *V << "]\n");
    }
    if (Name.startswith("__isoc99_scanf") || Name.startswith( "scanf") ||
            Name.startswith("ttt___isoc99_scanf") || Name.startswith( "ttt_scanf")) {
        for (unsigned i = 1, eee = CI->getNumArgOperands(); i
             != eee; ++i) { // skip format string (i=1)
            V = CI->getArgOperand(i);
            if (V->getType()->isPointerTy()) {
        	Input *i = new Input(V,0,programId);
		i->setInstName(Name);
                inputDep->insert(i);
                inserted = true;
                //				DEBUG(errs() << "Input [" << *V << "]\n");
            }
        }
    }else if (Name.equals("__isoc99_fscanf") || Name.equals("fscanf") ||
              Name.equals("ttt___isoc99_fscanf") || Name.equals("ttt_fscanf")) {
        for (unsigned i = 2, eee = CI->getNumArgOperands(); i
             != eee; ++i) { // skip file pointer and format string (i=1)
            V = CI->getArgOperand(i);
            if (V->getType()->isPointerTy()) {
        	Input *i = new Input(V,0,programId);
	    	i->setInstName(Name);
                inputDep->insert(i);
                inserted = true;
                //				DEBUG(errs() << "Input [" << *V << "]\n");
            }
        }
    }else if (Name.equals("gets") || Name.equals("fgets") || Name.equals("fread") || Name.equals("getwd") || Name.equals("getcwd")  || Name.equals("system")  ||
              Name.equals("ttt_gets") || Name.equals("ttt_fgets") || Name.equals("ttt_fread") || Name.equals("ttt_getwd") || Name.equals("ttt_getcwd")|| Name.equals("ttt_system")) {
        V = CI->getArgOperand(0); //the first argument receives the input for these functions
        if (V->getType()->isPointerTy()) {
            Input *i = new Input(V,0,programId);
            inputDep->insert(i);
	    i->setInstName(Name);
            inserted = true;
            //			DEBUG(errs() << "Input [" << *V << "]\n");
        }
   } else if (Name.equals("sensors_changed") || Name.equals("ttt_sensors_changed")){
            	DEBUG(errs() << "Sensors Changed Call ["<<*CI<< "] Name ["<<Name<<"] numArguments ["<<CI->getNumArgOperands()<<"]\n");
                V = CI->getArgOperand(0); //the first argument receives the input for these functions
            	if (V->getType()->isPointerTy() ) {
            		Input *i = new Input(V,0,programId);
               		inputDep->insert(i);
	    		i->setInstName(Name);
               		inserted = true;
            		DEBUG(errs() << "Sensors Changed: Input [" << *V << "] name [" << V->getName() <<"]\n");
		}
    }/* else if (Name.equals("packetbuf_dataptr") || Name.equals("ttt_packetbuf_dataprt")){
            	DEBUG(errs() << "Rime/Mesh packetbuf ptr ["<<*CI<< "] Name ["<<Name<<"] numArguments ["<<CI->getNumArgOperands()<<"]\n");
                V = CI->getArgOperand(0); //the first argument receives the input for these functions
            	if (V->getType()->isPointerTy() ) {
            		Input *i = new Input(V,1,programId);
               		inputDep->insert(i);
	    		i->setInstName(Name);
               		inserted = true;
            		DEBUG(errs() << "Rime/Mesh packetbuf: Input [" << *V << "] name [" << V->getName() <<"]\n");
		}
    }*/ else if (Name.equals("process_post") || Name.equals("ttt_process_post")){
            	DEBUG(errs() << "Process Post ["<<Name<<"]\n");
        V = CI->getArgOperand(2); //the third argument receives the input for these functions
        if(isa<ConstantExpr>(V)){
	    const ConstantExpr *CE = dyn_cast<ConstantExpr>(V);
            DEBUG(errs() << "Constant Expr in process_post[" << *CE << "] \n");
            if (V->getType()->isPointerTy() ) {
               V = (cast<ConstantExpr>(V))->getOperand(0);
               if(V->getName().equals("process_thread_serial_line_process.buf") || V->getName().equals("ttt_process_thread_serial_line_process.buf")){
            		Input *i = new Input(V,0,programId);
               		inputDep->insert(i);
               		inserted = true;
            		DEBUG(errs() << "Serial: Input [" << *V << "] name [" << V->getName() <<"]\n");
		} else{
            		DEBUG(errs() << "Constant Expr, but no serial: Input [" << *V << "] name [" << V->getName() <<"]\n");
		}
             }
	     else {
            	DEBUG(errs() << "No serial: Input [" << *V << "] \n");
	     }
          } /*else {
                V = CI->getArgOperand(0); //the first argument receives the input for these functions
                const Type *Ty = V->getType();
                Value *V2 = V->stripPointerCasts();	
		DEBUG(errs() << "Type [" << *(V->getType()) << "] \n");
		DEBUG(errs() << "Type [" << *(V2->getType()) << "] \n");
 		if (isa<Instruction>(V)){ 
			Instruction *i = dyn_cast<Instruction>(V2);
            		DEBUG(errs() << "Instruction in process_post[" << *i <<"] \n");
            		//DEBUG(errs() << "User in process_post[" << *u << "] with operands[" << u->getNumOperands()<<"] operand ["<<V2->getOperand(0) <<"]\n");
		}
 		if (isa<User>(V)){ 
			User *u = dyn_cast<User>(V2);
            		//DEBUG(errs() << "User in process_post[" << *u << "] with operands[" << u->getNumOperands()<<"] \n");
			unsigned int numOperands = u->getNumOperands();
            		DEBUG(errs() << "User in process_post[" << *u << "] with operands[" << numOperands<<"] \n");
		        for(int i = 0; i < numOperands; i++){	
            			DEBUG(errs() << "\n-----\n3.User in process_post operand[" << i <<"] operand ["<<*(u->getOperand(i)) <<"]\n");
				Value *V3 = u->getOperand(i);
 				if (isa<User>(V3)){ 
					 u = dyn_cast<User>(V3);
            				//DEBUG(errs() << "User in process_post[" << *u << "] with operands[" << u->getNumOperands()<<"] \n");
					unsigned int numOperands = u->getNumOperands();
            				DEBUG(errs() << "4. User in process_post[" << *u << "] with operands[" << numOperands<<"] \n");
					if(numOperands > 0){
		        			for(int j = 0; j < numOperands; j++){	
							Value *V4 = u->getOperand(j);
							if(!V4->getName().empty()){
            							DEBUG(errs() << "4.User in process_post operand[" << j <<"] operand ["<< V4->getName()<<"]\n");
								if(V4->getName().equals("process_thread_tcpip_process") || V4->getName().equals("ttt_process_thread_tcpip_process") || 
									V4->getName().equals("process_thread_uaodv_process") || V4->getName().equals("ttt_process_thread_uaodv_process")){
								     if (!skipNetSources) {
									Value *V5 = u->getOperand(j-1);
            							        DEBUG(errs() << "5. Get Operand of process_thread_tcpip_process ["<< *V5<<"]\n");
							                if (V->getType()->isPointerTy()){
            							             DEBUG(errs() << "6. Insert the Operand of process_thread_tcpip_process in inputdep \n");
							                     inputDep->insert(V);
							                     netInputDepValues.insert(V);
									}
								      }
								}
							} else {
            							DEBUG(errs() << "4.User in process_post operand[" << j <<"] operand ["<< *V4<<"]\n");
							}
						}
					}
				}
			}
		}

 		if (Ty->isStructTy()) { // Handle structs
                	const StructType *StTy = dyn_cast<StructType>(Ty);
                	unsigned numElems = StTy->getNumElements();
            		DEBUG(errs() << "Struct Type in process_post[" << *V << "] with numElems " << numElems <<"\n");
        	}

		if(isa<GetElementPtrInst>(V2)){
			GetElementPtrInst *GEPI = dyn_cast<GetElementPtrInst>(V2);
                        Value *v = GEPI->getPointerOperand();
                        const PointerType *PoTy = cast<PointerType>(GEPI->getPointerOperandType());
                        const Type *Ty = PoTy->getElementType();	
            		DEBUG(errs() << "GetElementPtrInst in process_post[" << *v << "] \n");
		}
 		if (isa<BitCastInst>(V2)) {
            		DEBUG(errs() << "BitCast in process_post [" << *V2 << "] \n");
                        BitCastInst *BC = dyn_cast<BitCastInst>(V2);
			Value *v2 = BC->getOperand(0);
            		DEBUG(errs() << "Operand 0 in bitcast of process_post [" << *v2 << "] \n");
		} else {
            		DEBUG(errs() << "No BitCastInst nor GerElementPtrInst: [" << *V2 << "] \n");
		}
     

      }
    
*/
    } else if (Name.equals("lll_read")) {//used to distinct between "read" used to read file or terminal and read network
        V = CI->getArgOperand(0); //the first argument receives the input for these functions
        if (V->getType()->isPointerTy()) {
          	Input *i = new Input(V,0,programId);
		i->setInstName(Name);
        	inputDep->insert(i);
        	inserted = true;
        //			DEBUG(errs() << "Input [" << *V << "]\n");
	}
    }else if (Name.equals("fgetc") || Name.equals("getc") || Name.equals("getchar") ||
              Name.equals("ttt_fgetc") || Name.equals("ttt_getc") || Name.equals("ttt_getchar")) {
          	Input *i = new Input(V,0,programId);
		i->setInstName(Name);
        	inputDep->insert(i);
        	inserted = true;
        //		DEBUG(errs() << "Input [" << *CI << "]\n");
    } else if (!skipNetSources) {
        if (Name.startswith("rrr_")) {
            if(RecvFuncArg > 0){
                V = CI->getArgOperand(RecvFuncArg); //the first argument receives the input for these functions
                if (V->getType()->isPointerTy()){
          	    Input *i = new Input(V,1,programId);
		    i->setInstName(Name);
                    inputDep->insert(i);
                    netInputDepValues.insert(i);
		}
            }else if(RecvFuncArg == 0){
          	    Input *i = new Input(CI,1,programId);
		    i->setInstName(Name);
                    inputDep->insert(i);
                    netInputDepValues.insert(i);
            }
            //			DEBUG(errs() << "Input [" << *V << "]\n");
            inserted = true;
        }else if (Name.startswith("ttt_rrr_")) {
            if(RecvFuncArg2 > 0){
                V = CI->getArgOperand(RecvFuncArg2); //the first argument receives the input for these functions
                if (V->getType()->isPointerTy()) {
          	    Input *i = new Input(V,1,programId);
		    i->setInstName(Name);
                    inputDep->insert(i);
                    netInputDepValues2.insert(i);
                }
            }else if(RecvFuncArg2 == 0){
          	Input *i = new Input(CI,1,programId);
		i->setInstName(Name);
                inputDep->insert(i);
                netInputDepValues2.insert(i);
            }
            //			DEBUG(errs() << "Input [" << *V << "]\n");
            inserted = true;
        }
    }
    /*	if (inserted) {
        if (MDNode *mdn = I->getMetadata("dbg")) {
            NumInputValues++;
            DILocation Loc(mdn);
            unsigned Line = Loc.getLineNumber();
            lineNo[V] = Line;
        }
    }
*/
}
bool InputDep::runOnModule(Module &M) {
    //	DEBUG (errs() << "Function " << F.getName() << "\n";);
    NumInputValues = 0;
    Function* main = M.getFunction("main");
    if (main) {
        MDNode *mdn = main->begin()->begin()->getMetadata("dbg");
        for (Function::arg_iterator Arg = main->arg_begin(), aEnd =
             main->arg_end(); Arg != aEnd; Arg++) {
	    Input *i = new Input(Arg, 0, 1);
	    i->setInstName("main");
            inputDepValues.insert(i);
            inputDepValuesWithNetSources.insert(i);
            NumInputValues++;
            DEBUG(errs()<<"Main arg["<<i->getValue()->getName() << "]\n");
	    
//            if (mdn) {
//                DILocation Loc(mdn);
//                unsigned Line = Loc.getLineNumber();
//                lineNo[Arg] = Line-1; //educated guess (can only get line numbers from insts, suppose main is declared one line before 1st inst
//            }
        }


    }
    Function* tmain = M.getFunction("ttt_main");
    if (tmain) {
        MDNode *mdn = tmain->begin()->begin()->getMetadata("dbg");
        for (Function::arg_iterator Arg = tmain->arg_begin(), aEnd =
             tmain->arg_end(); Arg != aEnd; Arg++) {
	    Input *i = new Input(Arg, 0, 2);
	    i->setInstName("main");
            inputDepValues2.insert(i);
            inputDepValues2WithNetSources.insert(i);
            NumInputValues++;
            DEBUG(errs()<<"Man arg["<<i->getValue()->getName() << "]\n");
 //           if (mdn) {
  //              DILocation Loc(mdn);
   //             unsigned Line = Loc.getLineNumber();
    //            lineNo[Arg] = Line-1; //educated guess (can only get line numbers from insts, suppose main is declared one line before 1st inst
     //       }
        }


    }
    bool ttt=false;
    for (Module::iterator F = M.begin(), eM = M.end(); F != eM; ++F) {

        for (Function::iterator BB = F->begin(), e = F->end(); BB != e; ++BB) {
            //			DEBUG(errs()<<F->getName() << "\n");
            std::string prefix = "ttt";
            std::string name = F->getName();
            if(name.substr(0,prefix.size())== prefix) ttt = true;
            else ttt=false;
            for (BasicBlock::iterator I = BB->begin(), ee = BB->end(); I != ee; ++I) {
//%receive_callback = getelementptr inbounds i8* %data, i64 16, !dbg !20743

	 	StringRef instName = I->getName().str();
        	if(instName.equals("receive_callback")) {
            		DEBUG(errs() << "1. Receive Instruction: "<< *I << "] Name:["<< instName << "]\n");
                        User *u = dyn_cast<User>(I);
                        //DEBUG(errs() << "User in process_post[" << *u << "] with operands[" << u->getNumOperands()<<"] \n");
                        unsigned int numOperands = u->getNumOperands();
                        DEBUG(errs() << "2. User in receive_callback[" << *u << "] with operands[" << numOperands<<"] \n");
                        //for(int i = 0; i < numOperands; i++){
                        for(int i = 0; i < 1; i++){
				Value *V = u->getOperand(i);
                                DEBUG(errs() << "\n-----\n3.User in receive_callback operand[" << i <<"] operand ["<< *V <<"]\n");
				if (V->getType()->isPointerTy()){
					if(ttt){
            					DEBUG(errs() << "4. ttt Insert the Operand of receive_callback in inputdep ["<< *V<< "]\n");
	    					Input *i = new Input(V, 1, 2);
						i->setInstName("receive_callback");
						inputDepValues2WithNetSources.insert(i);
						netInputDepValues2.insert(i);
					}else{
            					DEBUG(errs() << "4. Insert the Operand of receive_callback in inputdep ["<< *V<< "]\n");
	    					Input *i = new Input(V, 1, 1);
						i->setInstName("receive_callback");
						inputDepValuesWithNetSources.insert(i);
						netInputDepValues.insert(i);
					}
				}
			}

		} else if(instName.startswith("sensors_changed")){
           		DEBUG(errs() << "1. Sensor Instruction: "<< *I << "] Name:["<< instName << "]\n");
                        User *u = dyn_cast<User>(I);
                        unsigned int numOperands = u->getNumOperands();
                        DEBUG(errs() << "2. User in sensor instruction[" << *u << "] with operands[" << numOperands<<"] \n");
                        for(int i = 0; i < numOperands; i++){
				Value *V = u->getOperand(i);
                                DEBUG(errs() << "\n-----\n3.User in sensor instruction operand[" << i <<"] operand ["<< *V <<"]\n");
				if (V->getType()->isPointerTy()){
            				DEBUG(errs() << "4. Insert the Operand of sensor instruction in inputdep ["<< *V<< "]\n");
					if(ttt){
	    					Input *i = new Input(V, 0, 2);
						inputDepValues2WithNetSources.insert(i);
						inputDepValues2.insert(i);
					}else{
	    					Input *i = new Input(V, 1, 1);
						inputDepValuesWithNetSources.insert(i);
						inputDepValues.insert(i);
					}
				}
			}
 			
 
                } else if (CallInst *CI = dyn_cast<CallInst>(I)) {
                    Function *Callee = CI->getCalledFunction();
                    if (Callee) {
                        if(ttt) {
                            processInput(Callee,CI,&inputDepValues2WithNetSources, 2, false);
                            //	DEBUG(errs()<< "inputDepValues2WithNetSources.size()"<<inputDepValues2WithNetSources.size()<<"\n");
                            processInput(Callee,CI,&inputDepValues2, 2, true);
                            //	DEBUG(errs()<< "inputDepValues2.size()"<<inputDepValues2.size()<<"\n");
                        }
                        else {
                            processInput(Callee,CI,&inputDepValuesWithNetSources, 1, false);
                            //	DEBUG(errs()<< "inputDepValuesWithNetSources.size()"<<inputDepValuesWithNetSources.size()<<"\n");
                            processInput(Callee,CI,&inputDepValues, 1, true);
                            //	DEBUG(errs()<< "inputDepValues.size()"<<inputDepValues.size()<<"\n");
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
    errs() << "===Sources 1:====\n";
    for (std::set<Input*>::iterator i = inputDepValues.begin(), e =
         inputDepValues.end(); i != e; ++i) {
        errs() << *((*i)->getValue()) << "\n";
    }
    errs() << "===Sources 1 Net sources:====\n";
    for (std::set<Input*>::iterator i = netInputDepValues.begin(), e =
         netInputDepValues.end(); i != e; ++i) {
        errs() << *((*i)->getValue())<< "\n";
    }
    errs() << "===Sources 1 with net sources:====\n";
    for (std::set<Input*>::iterator i = inputDepValuesWithNetSources.begin(), e =
         inputDepValuesWithNetSources.end(); i != e; ++i) {
        errs() << *((*i)->getValue())<< "\n";
    }
    errs() << "===Sources 2:====\n";
    for (std::set<Input*>::iterator i = inputDepValues2.begin(), e =
         inputDepValues2.end(); i != e; ++i) {
        errs() << *((*i)->getValue())<< "\n";
    }
    errs() << "===Sources 2 Net sources:====\n";
    for (std::set<Input*>::iterator i = netInputDepValues2.begin(), e =
         netInputDepValues2.end(); i != e; ++i) {
        errs() << *((*i)->getValue())<< "\n";
    }
    errs() << "===Sources 2 with net sources:====\n";
    for (std::set<Input*>::iterator i = inputDepValues2WithNetSources.begin(), e =
         inputDepValues2WithNetSources.end(); i != e; ++i) {
        errs() << *((*i)->getValue())<< "\n";
    }
    errs() << "==============================\n";
}

void InputDep::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
}


//TODO: choose the program target. This version works with only 2 programs
std::set<Input*> InputDep::getInputDepValues(int programId) {
    if(programId == 1)
        return inputDepValues;
    else return inputDepValues2;

}
std::set<Input*> InputDep::getNetInputDepValues(int programId) {
    if(programId == 1)
        return netInputDepValues;
    else return netInputDepValues2;

}

std::set<Input*> InputDep::getInputDepValuesWithNetSources(int programId) {
    if(programId == 1) return inputDepValuesWithNetSources;
    else return inputDepValues2WithNetSources;
}
char InputDep::ID = 0;
static RegisterPass<InputDep> X("inputDep",
                                "Input Dependency Pass: looks for values that are input-dependant");
