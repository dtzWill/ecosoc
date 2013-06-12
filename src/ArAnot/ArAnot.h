#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "ArraySizeInference.h"

using namespace llvm;
using std::string;
using std::vector;

class ArAnot : public ModulePass {
  public:
    static char ID;
    ArAnot() : ModulePass(ID) { }

    virtual bool runOnModule(Module &M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    // virtual void print(raw_ostream& OS, const Module *) const;

  private:
    ASI *AS;
    DenseMap<unsigned, vector<string>*> Comments;
    string InputFile;

    string getSizeRepr(Value *V);
    string getFilename(Value *V);
    string getPtrTypeMetadata(Instruction *I);
    string getIndMetadata(Instruction *I);
    string getPtrMetadata(Instruction *I);
    string getPtrAccessRepr(Instruction *I);
    string getLineForIns(Value *V);
    int getLineNo(Value *V);
    void printToFile(string InputFile, string Filename);
    void addCommentToLine(string Comment, unsigned Line);
    void addComments(GetElementPtrInst *GEP);
};

class ArAnotMetadata : public ModulePass {
  public:
    static char ID;
    ArAnotMetadata() : ModulePass(ID) { }

    virtual bool runOnModule(Module &M);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};

