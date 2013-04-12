#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/AliasSetTracker.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/ADT/ilist.h"
#include "llvm/BasicBlock.h"
#include "llvm/Function.h"
#include "llvm/Module.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/PADriver.h"
#include "llvm/Support/Debug.h"


#include <cassert>
#include<set>
#include<map>
#include<queue>

class AliasSets : public ModulePass {
	std::map< int, std::set<Value*> > finalValueSets;
	std::map< int, std::set<int> > finalMemSets;
	std::map<int, std::set<int> > intSets;
	std::map<int, std::set<int> > intPointsTo;
	bool runOnModule(Module &M);
	void printSets();
	void printIntSets(std::set<int> to_erase);
	void mergeSets(int i, int j);
	bool analyzeSets();
	
	public:
	static char ID;
	AliasSets();
	void getAnalysisUsage(AnalysisUsage &AU) const;
	std::map< int, std::set<Value*> > getValueSets();
	std::map< int, std::set<int> > getMemSets();
	int getValueSetKey(Value* v);
	int getMapSetKey (int m);
};
