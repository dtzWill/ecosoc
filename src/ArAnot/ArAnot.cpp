#define DEBUG_TYPE "aranot"

#include "llvm/DebugInfo.h"
#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLibraryInfo.h"

#include "ArAnot.h"
#include <string>

using namespace llvm;

using std::string;
using std::fstream;
using std::ostream;

static cl::opt<string> ClOutputFile("aranot-output",
                                         cl::Hidden, cl::desc("Output file"));

void ArAnot::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<ASI>();
  AU.setPreservesAll();
}

// getLineNo
// Returns the line of a value given by its debug information.
int ArAnot::getLineNo(Value *V) {
  if (Instruction *I = dyn_cast<Instruction>(V))
    if (MDNode *N = I->getMetadata("dbg"))
      return DILocation(N).getLineNumber();
  return -1;
}

// getFilename
// Returns the file that the value is declared in.
string ArAnot::getFilename(Value *V) {
  if (Instruction *I = dyn_cast<Instruction>(V))
    if (MDNode *N = I->getMetadata("dbg"))
      return DILocation(N).getFilename();
  return string();
}

// getLineForIns
// Returns the line where the value is declared.
string ArAnot::getLineForIns(Value *V) {
  if (Instruction *I = dyn_cast<Instruction>(V)) {
    if (MDNode *N = I->getMetadata("dbg")) {
      DILocation Loc(N);
      unsigned Line = Loc.getLineNumber();
      StringRef File = Loc.getFilename();
      StringRef Dir = Loc.getDirectory();
      DIVariable Var(N);
      fstream FileStream;
      FileStream.open((Dir.str() + "/" + File.str()).c_str(), std::ios::in);
      for (unsigned i = 0; i < Line - 1; ++i)
        FileStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      string line;
      std::getline(FileStream, line);
      return line;
    }
  }
  return string();
}

// getSizeRepr
// Returns a string representation for a value.
string ArAnot::getSizeRepr(Value *V) {
  if (ConstantInt *CI = dyn_cast<ConstantInt>(V))
    // Return the string in base 10.
    return CI->getValue().toString(10, true);
  if (Instruction *I = dyn_cast<Instruction>(V)) {
    // Return the index name metadata if it has one.
    string Str = getIndMetadata(I);
    if (!Str.empty())
      return Str;
  }
  return string();
}

// getIndMetadata
string ArAnot::getIndMetadata(Instruction *I) {
  MDNode *IndexMd = I->getMetadata("ind_name");
  if (MDString *IndexNameMd = dyn_cast<MDString>(IndexMd->getOperand(2)))
    return IndexNameMd->getString();
  return string();
}

// getPtrTypeMetadata
string ArAnot::getPtrTypeMetadata(Instruction *I) {
  MDNode *IndexMd = I->getMetadata("ptr_type");
  if (MDString *IndexNameMd = dyn_cast<MDString>(IndexMd->getOperand(0)))
    return IndexNameMd->getString();
  return string();
}

// getPtrMetadata
string ArAnot::getPtrMetadata(Instruction *I) {
  MDNode *PtrMd = I->getMetadata("ptr_name");
  if (MDString *PtrNameMd = dyn_cast<MDString>(PtrMd->getOperand(2)))
    return PtrNameMd->getString();
  return string();
}

// getPtrAccessRepr
string ArAnot::getPtrAccessRepr(Instruction *I) {
  string Line = getLineForIns(I);
  string::iterator It = Line.begin(), E = Line.end();;
  // Skip all the blanks and tabs.
  for (; It != E; ++It)
    if (*It != ' ' && *It != '\t')
      break;
  string::iterator LIt = It;
  if (*It == '*') {
    // Pointer dereferences:
    // *ptr 
    // *(..)
  } else {
    // Array indexing operations:
    // ptr[i]
    while (*It != ']') ++It;
    return "&" + string(LIt, It + 1);
  }
  return string();
}

// addCommentToLine
void ArAnot::addCommentToLine(string Comment, unsigned Line) {
  if (!Comments.count(Line))
    Comments[Line] = new std::vector<string>();
  Comments[Line]->push_back(Comment);
}

// printToFile
void ArAnot::printToFile(string Input, string Output) {
  ifstream Infile(Input.c_str());
  string ErrorInfo;
  raw_fd_ostream File(Output.c_str(), ErrorInfo);
  errs() << "Writing output to file " << Output << "\n";

  string Line;
  unsigned LineNo = 1;
  while (!Infile.eof()) {
    std::getline(Infile, Line);
    if (Comments.count(LineNo)) {
      string Start;
      // Gather all the blanks and tabs.
      for (string::iterator It = Line.begin(), E = Line.end(); It != E; ++It)
        if (*It == ' ' || *It == '\t')
          Start += *It;
        else
          break;
      std::vector<string> *Vec = Comments[LineNo];
      unsigned MaxLen = 0;
      // Get the maximum length of all the comments.
      for (std::vector<string>::iterator It = Vec->begin(), E = Vec->end(); It != E; ++It)
        if (MaxLen < (*It).length())
          MaxLen = (*It).length();
      // Emit the comments.
      for (std::vector<string>::iterator It = Vec->begin(), E = Vec->end(); It != E; ++It) {
        File << Start << "/* " << *It << string(MaxLen - (*It).length(), ' ') << " */\n";
      }
    }
    File << Line << "\n";
    LineNo++;
  }
  File.close();
}

// addComments
// Add warning comments to GetElementPtrInst instructions.
void ArAnot::addComments(GetElementPtrInst *GEP) {
  unsigned Line = getLineNo(GEP);
  string Comment;
  string PointerName = getPtrAccessRepr(GEP);
  raw_string_ostream Stream(Comment);
  if (PointerName.empty()) {
    Stream << "WARNING: Possibly unsafe access.";
    addCommentToLine(Stream.str(), Line);
    return;
  }
  Value *PointerByteSize = AS->getByteSize(GEP);
  Value *PointerIndexSize = AS->getIndexSize(GEP);
  if (!AS->isValidSize(PointerByteSize) &&
      !AS->isValidSize(PointerIndexSize)) {
    Stream << "WARNING: Possibly unsafe memory access.";
    addCommentToLine(Stream.str(), Line);
    Comment.clear();
    Stream << "         Could not infer size for array `" << PointerName << "`.";
    addCommentToLine(Stream.str(), Line);
  } else {
    string PointerByteSizeRepr = getSizeRepr(PointerByteSize);
    string PointerIndexSizeRepr = getSizeRepr(PointerIndexSize);
    string PointerTypeRepr = getPtrTypeMetadata(GEP);
    if (!PointerIndexSizeRepr.empty() && !PointerTypeRepr.empty()) {
      Stream << "bytes(" << PointerName << ") = " << PointerIndexSizeRepr << " * sizeof(" << PointerTypeRepr << ")";
      addCommentToLine(Stream.str(), Line);
      Comment.clear();
      Stream << "index(" << PointerName << ") = " << PointerIndexSizeRepr;
      addCommentToLine(Stream.str(), Line);
      Comment.clear();
      Stream << "WARNING: Possibly unsafe access.";
      addCommentToLine(Stream.str(), Line);
      if (PointerIndexSizeRepr == "0") {
        Comment.clear();
        Stream << "         Off-by-one error.";
        addCommentToLine(Stream.str(), Line);
      }
      return;
    } else if (!PointerByteSizeRepr.empty()) {
      Stream << "bytes(" << PointerName << ") = " << PointerByteSizeRepr;
      addCommentToLine(Stream.str(), Line);
      Comment.clear();
    }
    Stream << "WARNING: Possibly unsafe access.";
    addCommentToLine(Stream.str(), Line);
  }
}

bool ArAnot::runOnModule(Module &M) {
  AS = &getAnalysis<ASI>();

  for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F) {
    if (F->isDeclaration() || F->isIntrinsic())
      continue;
    for (Function::iterator BB = F->begin(), BE = F->end(); BB != BE; ++BB)
      for (BasicBlock::iterator I = BB->begin(), IE = BB->end(); I != IE; ++I) {
        if (InputFile.empty())
          InputFile = getFilename(&(*I));
        if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(&(*I)))
          if (!AS->isSafe(GEP))
            addComments(GEP);
      }
  }

  if (!InputFile.empty() && !ClOutputFile.empty())
    printToFile(InputFile, ClOutputFile);
  
  return false;
}

char ArAnot::ID = 0;
static RegisterPass<ArAnot> X("aranot", "Array annotator");

// Propagates debug metadata throughout the CFG.
bool ArAnotMetadata::runOnModule(Module &M) {
  Function *DbgValueFn = M.getFunction("llvm.dbg.declare");
  for (Module::iterator F = M.begin(), FE = M.end(); F != FE; ++F) {
    if (F->isDeclaration() || F->isIntrinsic())
      continue;
    for (Function::iterator BB = F->begin(), BE = F->end(); BB != BE; ++BB)
      for (BasicBlock::iterator I = BB->begin(), IE = BB->end(); I != IE; ++I) {
        if (CallInst *CI = dyn_cast<CallInst>(&(*I))) {
          if (Function *Called = CI->getCalledFunction()) {
            if (Called != DbgValueFn)
              continue;
            MDNode *Metadata0 = cast<MDNode>(CI->getArgOperand(1));
            Value *TypeString = NULL;
            MDNode *TypeStringNode = NULL;
            if (Metadata0->getNumOperands() > 5) {
              MDNode *Metadata1 = dyn_cast<MDNode>(Metadata0->getOperand(5));
              if (Metadata1 && Metadata1->getNumOperands() > 9) {
                MDNode *Metadata2 = dyn_cast<MDNode>(Metadata1->getOperand(9));
                if (Metadata2 && Metadata2->getNumOperands() > 2) {
                  TypeString = Metadata2->getOperand(2);
                  TypeStringNode =
                    MDNode::get(I->getContext(), ArrayRef<Value*>(TypeString));
                }
              }
            }
            Value *V = cast<MDNode>(CI->getArgOperand(0))->getOperand(0);
            for (Value::use_iterator UI = V->use_begin(), UE = V->use_end(); UI != UE; ++UI) {
              if (LoadInst *LI = dyn_cast<LoadInst>(*UI)) {
                LI->setMetadata("ind_name", Metadata0);
                LI->setMetadata("ptr_name", Metadata0);
                if (TypeString)
                  LI->setMetadata("ptr_type", TypeStringNode);
              }
            }
          }
        } else if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(&(*I))) {
          Instruction *Ptr = dyn_cast<Instruction>(GEP->getPointerOperand());
          Instruction *Index = dyn_cast<Instruction>(GEP->getOperand(GEP->getNumOperands() - 1));
          if (Ptr && Ptr->getMetadata("ptr_name"))
            GEP->setMetadata("ptr_name", Ptr->getMetadata("ptr_name"));
          if (Ptr && Ptr->getMetadata("ptr_type"))
            GEP->setMetadata("ptr_type", Ptr->getMetadata("ptr_type"));
          if (Index && Index->getMetadata("ind_name"))
            GEP->setMetadata("ind_name", Index->getMetadata("ind_name"));
        } else if (isa<CastInst>(&(*I))) {
          Instruction *Index = dyn_cast<Instruction>(I->getOperand(0));
          if (Index && Index->getMetadata("ptr_name"))
            I->setMetadata("ptr_name", Index->getMetadata("ptr_name"));
          if (Index && Index->getMetadata("ptr_type"))
            I->setMetadata("ptr_type", Index->getMetadata("ptr_type"));
          if (Index && Index->getMetadata("ind_name"))
            I->setMetadata("ind_name", Index->getMetadata("ind_name"));
        }
      }
  }

  return false;
}

void ArAnotMetadata::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

char ArAnotMetadata::ID = 0;
static RegisterPass<ArAnotMetadata> Y("aranot-metadata", "Array annotator metadata propagator");

