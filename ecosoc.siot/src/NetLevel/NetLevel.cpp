#define DEBUG_TYPE "NetLevel"

#include "NetLevel.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"

#include "llvm/IR/User.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/ilist.h"
#include "llvm/DebugInfo.h"
#include "llvm/DebugInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/ADT/ilist.h"
#include "llvm/Support/CommandLine.h"

using namespace llvm;

static cl::opt<std::string> OutputDir("OutputDir", cl::desc("Specify output dir"), cl::value_desc("outputdir"));
static cl::opt<std::string> P1Func("P1FUNC", cl::desc("Specify program 1 entry function"), cl::value_desc("p1function"));
static cl::opt<std::string> P2Func("P2FUNC", cl::desc("Specify program 2 entry function"), cl::value_desc("p2function"));

template<typename GraphType> char NetLevel<GraphType>::ID = 0;
static RegisterPass<NetLevel<Function*> > X("NetLevel", "Get instructions levels");

template<typename GraphType> NetLevel<GraphType>::NetLevel() :
    ModulePass(ID) {
}

template<typename GraphType> bool NetLevel<GraphType>::runOnModule(Module &M) {       

    if(OutputDir.empty()) {
        outputDir = "";
    } else {
        outputDir = OutputDir;
    }
    if(P1Func.empty()) {
        program1EntryFunction = "ttt_main";
    } else {
        program1EntryFunction = P1Func;
    }
    if(P2Func.empty()) {
        program2EntryFunction = "main";
    } else {
        program2EntryFunction = P2Func;
    }

    this->getModules(M);

    writeCFGsToFiles();

    createLevels();

    return false;
}

template<typename GraphType> void NetLevel<GraphType>::getModules(Module &M) {
    for (Module::iterator Fit = M.begin(), Fend = M.end(); Fit != Fend;
         ++Fit) {
        StringRef functionName = Fit->getName();
        ProgramStructure *program = detectProgram(functionName);

        if (program->functionsMap.count(functionName.str() + ".entry") <= 0) {
            program->functionsMap[functionName.str() + ".entry"] = new FunctionInfo(functionName.str());
            errs() << "NetLevel new Function1 " << functionName.str() << "\n";
            if (functionName.find("sss_") != StringRef::npos
                    || functionName.find("rrr_") != StringRef::npos) {
                continue;
            }
        }

        list<Vertex> prevList;
        for (node_iterator I = GTraits::nodes_begin(Fit), E =
             GTraits::nodes_end(Fit); I != E; ++I) {
            Vertex u2 = -1;
            string cfgNodeName = functionName.str() + "."
                    + I->getName().str();
            if (program->nodesMap.count(cfgNodeName) <= 0) {
                FunctionInfo *f = program->functionsMap[functionName.str()+".entry"];
                u2 = GraphStructure::createVertex(f->graph, cfgNodeName, boost::gray_color);
            } else {
                u2 = program->nodesMap[cfgNodeName];
            }

            prevList.clear();
            prevList.push_back(u2);
            program->nodesMap[cfgNodeName] = u2;

            Vertex lastNet = -1;

            getInstructions(program, *I, prevList, lastNet, functionName);
            if (lastNet != -1) {
                prevList.clear();
                prevList.push_back(lastNet);
            }

            // iterate over all of the edges
            child_iterator EI = GTraits::child_begin(I);
            child_iterator EE = GTraits::child_end(I);
            for (unsigned i = 0; EI != EE && i != 64; ++EI, ++i) {
                Vertex u3 = -1;
                string cfgChildNodeName = functionName.str() + "."
                        + EI->getName().str();
                if (program->nodesMap.count(cfgChildNodeName) <= 0) {
                    FunctionInfo * f = program->functionsMap[functionName.str() + ".entry"];
                    u3 = GraphStructure::createVertex(f->graph, cfgChildNodeName, boost::red_color);
                    program->nodesMap[cfgChildNodeName] = u3;
                } else {
                    u3 = program->nodesMap[cfgChildNodeName];
                }

                Vertex u1 = u2;
                if (lastNet != -1) {
                    u1 = lastNet;
                }

                Edge e1;
                //errs() << " u1 " << u1 << " u3 " << u3 << "\n";
                e1 = (add_edge(u1, u3, program->functionsMap[functionName.str() + ".entry"]->graph)).first;
                (program->functionsMap[functionName.str() + ".entry"]->distances)[e1] = 1;
            }
        }
    }
}

template<typename GraphType> void NetLevel<GraphType>::getInstructions(ProgramStructure *program, BasicBlock &B, list<Vertex> u1list, Vertex &retVert, const string &functionName) {
    Vertex firstVertex = -1;
    Vertex lastVertex = -1;
    for (BasicBlock::iterator i = B.begin(), e = B.end(); i != e; ++i) {
        CallInst* callInst = dyn_cast<CallInst>(i);
        if (isa<CallInst>(i) && callInst->getCalledFunction()) {
            StringRef instName =
                    callInst->getCalledFunction()->getName();
            if (instName.startswith("llvm.dbg")) {
                continue;
            }
            bool networkNode = false;
            if (instName.find("sss_") != StringRef::npos
                    || instName.find("rrr_") != StringRef::npos) {
                networkNode = true;
            }

            //errs() << " InstName:  " << instName << "\n";
            VertexExtraData *extraData = NULL;
            MDNode *N = NULL;
            if (N = callInst->getMetadata("dbg")) { // Here I is an LLVM instruction
                returnCodeInfo(N, &extraData);
            }
            Vertex u2 = GraphStructure::createVertex(program->functionsMap[functionName + ".entry"]->graph, instName, networkNode ? boost::green_color : boost::white_color, extraData);
            delete extraData;
            if (program->functionsMap.count(instName.str() + ".entry") <= 0) {
                program->functionsMap[instName.str() + ".entry"] = new FunctionInfo(instName.str());
                errs() << "NetLevel new Function2 " << instName.str() << "\n";
            }
            if (!networkNode) {
                program->functionsMap[functionName + ".entry"]->addUsedFunction(u2, program->functionsMap[instName.str() + ".entry"]);
            }
            program->functionsMap[instName.str() + ".entry"]->addUser(program->functionsMap[functionName + ".entry"]);
            // Create the edges
            for (std::list<Vertex>::iterator it = u1list.begin(); it != u1list.end(); ++it) {
                Edge e1;
                Vertex &u1 = (*it);
                //errs() << " u1 " << u1 << " u2 " << u2 << "\n";
                e1 = (add_edge(u1, u2, program->functionsMap[functionName + ".entry"]->graph)).first;
                (program->functionsMap[functionName + ".entry"]->distances)[e1] = 1;
            }
            //errs() << " assign last= " << u2 << "\n";
            retVert = u2;
            u1list.clear();
            u1list.push_back(u2);
            lastVertex = u2;
            if (firstVertex == -1) {
                firstVertex = u2;
            }
        }
    }
}

template<typename GraphType> void NetLevel<GraphType>::createLevels() {
    errs() << "\n\nlevels for program 1 - sends\n";
    GraphStructure::createLevelsForGraph(program1.fullNetSendGraph->graph, program1.levelSetsSends);
    errs() << "\n\nlevels for program 1 - recvs\n";
    GraphStructure::createLevelsForGraph(program1.fullNetRecvGraph->graph, program1.levelSetsRecvs);
    errs() << "\n\nlevels for program 2 - sends\n";
    GraphStructure::createLevelsForGraph(program2.fullNetSendGraph->graph, program2.levelSetsSends);
    errs() << "\n\nlevels for program 2 - recvs\n";
    GraphStructure::createLevelsForGraph(program2.fullNetRecvGraph->graph, program2.levelSetsRecvs);
}

template<typename GraphType> void NetLevel<GraphType>::replaceFunctionVertices(FunctionInfo *functionInfo){
    errs() << "FunctionInfo REPVERT " << functionInfo->getFunctionName() << "\n";
    list<Vertex> removedNodeUsers;
    list<Vertex> maintainedNodeUsers;
    for (map<Vertex, FunctionInfo* >::reverse_iterator vfIt = functionInfo->usedFunctions.rbegin();
         vfIt != functionInfo->usedFunctions.rend(); ++vfIt) {
        Vertex v = (*vfIt).first;
        FunctionInfo *innerFunctionInfo = (FunctionInfo*)(*vfIt).second;
        // treat only not empty graphs
        if(num_vertices(innerFunctionInfo->graph) > 1 && functionInfo->getFunctionName() != innerFunctionInfo->getFunctionName()) {
            if(!innerFunctionInfo->usedFunctions.empty() && innerFunctionInfo->onCallHierarchy) {
                replaceFunctionVertices(innerFunctionInfo);
            }
            bool pastRemoved = false;
            for(list<Vertex>::iterator vIt = removedNodeUsers.begin(); vIt != removedNodeUsers.end(); ++vIt) {
                if(v > (*vIt) ) {
                    pastRemoved = true;
                    break;
                }
            }
            //functionInfo->writeToDot(std::cerr);
            functionInfo->replaceVertexWithGraph(pastRemoved ? v-1 : v, innerFunctionInfo->graph);            
            // now there is no dependency, mark to remove from users and replace vertex with graph
            removedNodeUsers.push_back(v);
        } else {
            maintainedNodeUsers.push_back(v);
        }
    }
    for(list<Vertex>::iterator it = removedNodeUsers.begin(); it != removedNodeUsers.end(); ++it) {
        functionInfo->usedFunctions.erase((Vertex)*it);
    }
    for(list<Vertex>::iterator it = maintainedNodeUsers.begin(); it != maintainedNodeUsers.end(); ++it) {
        functionInfo->usedFunctions.erase((Vertex)*it);
    }
}

template<typename GraphType> void NetLevel<GraphType>::writeCFGsToFiles() {
    set<FunctionInfo*> p1EntryFunctions;
    for (map<string, FunctionInfo* >::iterator it = program1.functionsMap.begin();
         it != program1.functionsMap.end(); ++it) {
        FunctionInfo* fi = (FunctionInfo*)(*it).second;
        if(fi->getFunctionName().find("rrr_") != string::npos) {
            ofstream fileProgram1RecvFunction((outputDir.str()+"/p1RecvFunction_"+fi->getFunctionName() + ".cst").c_str());
            fi->printCallStack(fileProgram1RecvFunction, 0, p1EntryFunctions);
            fileProgram1RecvFunction.close();
        } else {
            if(fi->getFunctionName().find("sss_") != string::npos) {
                ofstream fileProgramSendFunction((outputDir.str()+"/p1SendFunction_"+fi->getFunctionName() + ".cst").c_str());
                fi->printCallStack(fileProgramSendFunction, 0, p1EntryFunctions);
                fileProgramSendFunction.close();
            }
        }
    }
    set<FunctionInfo*> p2EntryFunctions;
    for (map<string, FunctionInfo* >::iterator it = program2.functionsMap.begin();
         it != program2.functionsMap.end(); ++it) {
        FunctionInfo* fi = (FunctionInfo*)(*it).second;
        if(fi->getFunctionName().find("rrr_") != string::npos) {
            ofstream fileProgram2RecvFunction((outputDir.str()+"/p2RecvFunction_"+fi->getFunctionName() + ".cst").c_str());
            fi->printCallStack(fileProgram2RecvFunction, 0, p2EntryFunctions);
            fileProgram2RecvFunction.close();
        } else {
            if(fi->getFunctionName().find("sss_") != string::npos) {
                ofstream fileProgram2RecvFunction((outputDir.str()+"/p2SendFunction_"+fi->getFunctionName() + ".cst").c_str());
                fi->printCallStack(fileProgram2RecvFunction, 0, p2EntryFunctions);
                fileProgram2RecvFunction.close();
            }
        }
    }

    for(set<FunctionInfo*>::iterator it = p1EntryFunctions.begin(); it != p1EntryFunctions.end(); ++it){
        FunctionInfo* fi = *it;
        errs() << "P1 ENTRY FUNCTION " << fi->getFunctionName() << "\n";
        ofstream fileProgram1((outputDir.str() + "/p1_" + fi->getFunctionName() + ".dot").c_str());
        replaceFunctionVertices(fi);
        fi->writeToDot(fileProgram1);
        fileProgram1.close();

        ofstream fileNetGraph((outputDir.str() + "/p1_ng_" + fi->getFunctionName() + ".dot").c_str());
        fi->writeOnlyNetToDot(fileNetGraph);
        fileNetGraph.close();

        ofstream fileNetSend((outputDir.str() + "/p1_ns_" + fi->getFunctionName() + ".dot").c_str());
        BoostGraph nsGraph = fi->writeOnlyPatternToDot(fileNetSend, "sss_");
        boost::copy_graph( nsGraph, program1.fullNetSendGraph->graph);
        fileNetSend.close();

        ofstream fileNetRecv((outputDir.str() + "/p1_nr_" + fi->getFunctionName() + ".dot").c_str());
        BoostGraph nrGraph = fi->writeOnlyPatternToDot(fileNetRecv, "rrr_");
        boost::copy_graph( nrGraph, program1.fullNetRecvGraph->graph);
        fileNetRecv.close();
    }
    ofstream fileP1FullNetSend((outputDir.str() + "/p1_full_ns" + ".dot").c_str());
    program1.fullNetSendGraph->writeToDot(fileP1FullNetSend);
    fileP1FullNetSend.close();
    ofstream fileP1FullNetRecv((outputDir.str() + "/p1_full_nr" + ".dot").c_str());
    program1.fullNetRecvGraph->writeToDot(fileP1FullNetRecv);
    fileP1FullNetRecv.close();

    for(set<FunctionInfo*>::iterator it = p2EntryFunctions.begin(); it != p2EntryFunctions.end(); ++it){
        FunctionInfo* fi = *it;
        errs() << "P2 ENTRY FUNCTION " << fi->getFunctionName() << "\n";
        ofstream fileProgram2((outputDir.str() + "/p2_" + fi->getFunctionName() + ".dot").c_str());
        replaceFunctionVertices(fi);
        fi->writeToDot(fileProgram2);
        fileProgram2.close();

        ofstream fileNetGraph((outputDir.str() + "/p2_ng_" + fi->getFunctionName() + ".dot").c_str());
        fi->writeOnlyNetToDot(fileNetGraph);
        fileNetGraph.close();

        ofstream fileNetSend((outputDir.str() + "/p2_ns_" + fi->getFunctionName() + ".dot").c_str());
        BoostGraph nsGraph = fi->writeOnlyPatternToDot(fileNetSend, "sss_");
        boost::copy_graph( nsGraph, program2.fullNetSendGraph->graph);
        fileNetSend.close();

        ofstream fileNetRecv((outputDir.str() + "/p2_nr_" + fi->getFunctionName() + ".dot").c_str());
        BoostGraph nrGraph = fi->writeOnlyPatternToDot(fileNetRecv, "rrr_");
        boost::copy_graph( nrGraph, program2.fullNetRecvGraph->graph);
        fileNetRecv.close();
    }
    ofstream fileP2FullNetSend((outputDir.str() + "/p2_full_ns" + ".dot").c_str());
    program2.fullNetSendGraph->writeToDot(fileP2FullNetSend);
    fileP2FullNetSend.close();
    ofstream fileP2FullNetRecv((outputDir.str() + "/p2_full_nr" + ".dot").c_str());
    program2.fullNetRecvGraph->writeToDot(fileP2FullNetRecv);
    fileP2FullNetRecv.close();
}

template<typename GraphType> void NetLevel<GraphType>::returnCodeInfo(MDNode *N, VertexExtraData **extraData) {
    DILocation Loc(N); // DILocation is in DebugInfo.h
    *extraData = new VertexExtraData();
    (*extraData)->codeLine = Loc.getLineNumber();
    (*extraData)->codeName = Loc.getDirectory().str() + '/' + Loc.getFilename().str();
}

template<typename GraphType> ProgramStructure * NetLevel<GraphType>::detectProgram(const StringRef &functionName) {
    if (functionName.startswith("ttt_")) {
        return &program2;
    } else {
        return &program1;
    }
}

template<typename GraphType> void NetLevel<GraphType>::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
    AU.setPreservesAll();
}
