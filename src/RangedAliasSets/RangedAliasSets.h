#ifndef __RANGED_ALIAS_SETS_H__
#define __RANGED_ALIAS_SETS_H__
#include "llvm/Pass.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/APInt.h"
#include <set>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../../AliasSets/AliasSets.h"
#include "../../RangeAnalysis/RangeAnalysis/RangeAnalysis.h"
using namespace std;

namespace llvm {

class RangedAliasSets: public ModulePass{
	private:
	//Verifies if the instruction verify is present in vector arr
	bool 
		isPresent
			(Instruction* verify, std::vector<Instruction*> arr);
	//Takes a vector of instructions (ptr) and orders them
	std::vector<Instruction*>
		orderInstructions
			(std::vector<Instruction*> unordered, Module* M);
	//Holds a Primitive Layout for a determined Type
	struct PrimitiveLayout
	{
		Type * type;
		std::vector<int> layout;
		PrimitiveLayout(Type* ty, std::vector<int> lay)
		{
			type = ty;
			layout = lay;
		}
	};
	struct NumPrimitive
	{
		Type * type;
		int num;
		NumPrimitive(Type* ty, int n)
		{
			type = ty;
			num = n;
		}
	};
	std::vector<PrimitiveLayout*> PrimitiveLayouts;
	std::vector<NumPrimitive*> NumPrimitives;
	std::vector<int> getPrimitiveLayout(Type* type);
	int getNumPrimitives(Type* type);
	llvm::Type* getTypeInside(Type* type, int i);
	int getSumBehind(std::vector<int> v, int i);
	//Holds a memory range for a determined Value
	struct MemRange 
	{
		Value* mem;
		Value* aloc;
		APInt lower;
		APInt higher;
		MemRange(Value* Mem, APInt Lower, APInt Higher, Value* Aloc){
			mem = Mem; lower = Lower; higher = Higher; aloc = Aloc;
		}
		//Finds the MemRange of element in a MemRange set
		static MemRange* 
			FindByValue
				(Value* element, std::set<MemRange*> MRSet);
	};
	//Persistent maps
	llvm::DenseMap<int, std::set<MemRange*> > MemRangeSets;
	llvm::DenseMap<int, std::set<MemRange*> > RangeAliasSets;
	llvm::DenseMap<int, std::set<Value*> > NewAliasSets;
	//Methods for debugging
	void printRangeAnalysis(InterProceduralRA<Cousot> *ra, Module *M);
	void printAliasSets(llvm::DenseMap<int, std::set<Value*> > *AliasSets);
	void printInterestingSets(llvm::DenseMap<int, std::set<Value*> > *InterestingSets);
	void printInterestingVectors(llvm::DenseMap<int, std::vector<Instruction*> > *InterestingVectors);
	void printMemRanges(llvm::DenseMap<int, std::set<MemRange*> > *MemRangeSets);
	void printRangeAliasSets(llvm::DenseMap<int, std::set<MemRange*> > *RangedAliasSets);
	void printNewAliasSets(llvm::DenseMap<int, std::set<Value*> > *NewAliasSets);
	void printPrimitiveLayouts(std::vector<PrimitiveLayout*> PrimitiveLayouts);
	
	public:
	//LLVM framework methods and atributes
	static char ID;
	RangedAliasSets() : ModulePass(ID) {}
	bool runOnModule(Module &M);
	void getAnalysisUsage(AnalysisUsage &AU) const;
	//methods that return the persistent maps
	llvm::DenseMap<int, std::set<MemRange*> > getRangedAliasSets();
	llvm::DenseMap<int, std::set<Value*> > getAliasSets();
};

}
#endif