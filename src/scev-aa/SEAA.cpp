//===- ScalarEvolutionAliasAnalysis.cpp - SCEV-based Alias Analysis -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the ScalarEvolutionAliasAnalysis pass, which implements a
// simple alias analysis implemented in terms of ScalarEvolution queries.
//
// This differs from traditional loop dependence analysis in that it tests
// for dependencies within a single iteration of a loop, rather than
// dependencies between different iterations.
//
// ScalarEvolution has a more complete understanding of pointer arithmetic
// than BasicAliasAnalysis' collection of ad-hoc analyses.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "../../AliasSets/AliasSets.h"
#include <set>
#include <cstdlib>
#include <cstring>
#include <vector>
using namespace llvm;
STATISTIC(NAliasSets, "Number of original alias sets");
STATISTIC(NInterestingSets, "Number of alias sets that were divided");
STATISTIC(NNewSets, "Number of alias sets found from the divided");
STATISTIC(NFinalSets, "Number of final alias sets");
STATISTIC(NNoAlias, "Number of no alias results");
STATISTIC(NMayAlias, "Number of may alias results");
STATISTIC(NPartialAlias, "Number of partial alias results");
STATISTIC(NMustAlias, "Number of must alias results");

namespace {
  /// ScalarEvolutionAliasAnalysis - This is a simple alias analysis
  /// implementation that uses ScalarEvolution to answer queries.
  class ScalarEvolutionAliasAnalysis : public FunctionPass,
                                       public AliasAnalysis {
    ScalarEvolution *SE;

  public:
    static char ID; // Class identification, replacement for typeinfo
    ScalarEvolutionAliasAnalysis() : FunctionPass(ID), SE(0) {
      initializeScalarEvolutionAliasAnalysisPass(
        *PassRegistry::getPassRegistry());
        NInterestingSets = 0;
        NNewSets = 0;
        NFinalSets = 0;
        NNoAlias = 0;
       NMayAlias = 0;
		NPartialAlias = 0;
		NMustAlias = 0;
    }

    /// getAdjustedAnalysisPointer - This method is used when a pass implements
    /// an analysis interface through multiple inheritance.  If needed, it
    /// should override this to adjust the this pointer as needed for the
    /// specified pass info.
    virtual void *getAdjustedAnalysisPointer(AnalysisID PI) {
      if (PI == &AliasAnalysis::ID)
        return (AliasAnalysis*)this;
      return this;
    }

  private:
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
    virtual bool runOnFunction(Function &F);
    virtual AliasResult alias(const Location &LocA, const Location &LocB);

    Value *GetBaseValue(const SCEV *S);
  };
}  // End of anonymous namespace

// Register this pass...
char ScalarEvolutionAliasAnalysis::ID = 0;
/*INITIALIZE_AG_PASS_BEGIN(ScalarEvolutionAliasAnalysis, AliasAnalysis, "seaa",
                   "ScalarEvolution-based Alias Analysis", false, true, false)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolution)
INITIALIZE_AG_PASS_END(ScalarEvolutionAliasAnalysis, AliasAnalysis, "seaa",
                    "ScalarEvolution-based Alias Analysis", false, true, false)
*/
static RegisterPass<ScalarEvolutionAliasAnalysis> X("seaa",
"MyScalarEvolutionAliasAnalysis", false, false);

FunctionPass *llvm::createScalarEvolutionAliasAnalysisPass() {
  return new ScalarEvolutionAliasAnalysis();
}

void
ScalarEvolutionAliasAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AliasSets>();
  AU.addRequiredTransitive<ScalarEvolution>();
  AU.setPreservesAll();
  AliasAnalysis::getAnalysisUsage(AU);
}
///////////////////////////////////////////////
bool
ScalarEvolutionAliasAnalysis::runOnFunction(Function &F) {
  InitializeAliasAnalysis(this);
  SE = &getAnalysis<ScalarEvolution>();
  
  AliasSets &AS = getAnalysis<AliasSets>();
  llvm::DenseMap<int, std::set<Value*> > AliasSets = AS.getValueSets();
  	////////Our Alias Sets
  	NAliasSets = 0;//AliasSets.size();//statistics
	for (llvm::DenseMap<int, std::set<Value*> >::iterator i = AliasSets.begin(), e = AliasSets.end(); 
	i != e; ++i) if(i->second.size() > 0) NAliasSets++;
	////////Finding interesting sets
	llvm::DenseMap<int, std::set<Value*> > InterestingSets;
	int ISi = 0;
	for (llvm::DenseMap<int, std::set<Value*> >::iterator aai = AliasSets.begin(), aae = AliasSets.end(); 
	aai != aae; ++aai){
		bool mark = true;
		for(std::set<Value*>::iterator si = aai->second.begin(), se = aai->second.end(); si != se; si++){
			bool mark2 = false;
			for(Function::iterator bb = F.begin(), bbEnd = F.end(); bb != bbEnd; ++bb){
				for(BasicBlock::iterator I = bb->begin(), IEnd = bb->end(); I != IEnd; ++I){
					if((*si) == I) mark2 = true;
				}
			}
			if(mark2 == false){
				mark = false;
				break;
			}
		}
		if((mark == true)&&(aai->second.size() > 1)){
				ISi++;
				InterestingSets[ISi] = aai->second;
		}
	}
	NInterestingSets += InterestingSets.size();
	
	/*//printing interesting sets
	errs() << "Insteresting Sets" << "\n";	
	for (llvm::DenseMap<int, std::set<Value*> >::iterator i = InterestingSets.begin(), e = InterestingSets.end(); 
	i != e; ++i) {
        errs() << "Set " << i->first << "\n";
        for (std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); 
        ii != ee; ++ii) {

            errs() << "	" << **ii << "\n";
        }

        errs() << "\n";
	}*/
	
	llvm::DenseMap<int, std::set<Value*> >::iterator i = InterestingSets.begin();
	
	llvm::DenseMap<int, Value* > list;
	int iii = 0;
	if(ISi > 0)
	for (std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); 
   ii != ee; ++ii) {
		errs() << **ii << "\n";
		list[iii] = *ii;
		iii++;
	}
	for(int j = 0; j < iii; j++){
		for(int w = j+1; w < iii; w++){
			if(alias(Location(list[j]),Location(list[w])) == NoAlias)
				NNoAlias++;
			else if (alias(Location(list[j]),Location(list[w])) == MayAlias)
				NMayAlias++;
			else if (alias(Location(list[j]),Location(list[w])) == PartialAlias)
				NPartialAlias++;
			else if (alias(Location(list[j]),Location(list[w])) == MustAlias)
				NMustAlias++;
		}
	}
  
  
  NFinalSets = (NAliasSets - NInterestingSets + NNewSets);
  return false;
}

/// GetBaseValue - Given an expression, try to find a
/// base value. Return null is none was found.
Value *
ScalarEvolutionAliasAnalysis::GetBaseValue(const SCEV *S) {
  if (const SCEVAddRecExpr *AR = dyn_cast<SCEVAddRecExpr>(S)) {
    // In an addrec, assume that the base will be in the start, rather
    // than the step.
    return GetBaseValue(AR->getStart());
  } else if (const SCEVAddExpr *A = dyn_cast<SCEVAddExpr>(S)) {
    // If there's a pointer operand, it'll be sorted at the end of the list.
    const SCEV *Last = A->getOperand(A->getNumOperands()-1);
    if (Last->getType()->isPointerTy())
      return GetBaseValue(Last);
  } else if (const SCEVUnknown *U = dyn_cast<SCEVUnknown>(S)) {
    // This is a leaf node.
    return U->getValue();
  }
  // No Identified object found.
  return 0;
}

AliasAnalysis::AliasResult
ScalarEvolutionAliasAnalysis::alias(const Location &LocA,
                                    const Location &LocB) {
  // If either of the memory references is empty, it doesn't matter what the
  // pointer values are. This allows the code below to ignore this special
  // case.
  if (LocA.Size == 0 || LocB.Size == 0)
    return NoAlias;

  // This is ScalarEvolutionAliasAnalysis. Get the SCEVs!
  const SCEV *AS = SE->getSCEV(const_cast<Value *>(LocA.Ptr));
  const SCEV *BS = SE->getSCEV(const_cast<Value *>(LocB.Ptr));

  // If they evaluate to the same expression, it's a MustAlias.
  if (AS == BS) return MustAlias;

  // If something is known about the difference between the two addresses,
  // see if it's enough to prove a NoAlias.
  if (SE->getEffectiveSCEVType(AS->getType()) ==
      SE->getEffectiveSCEVType(BS->getType())) {
    unsigned BitWidth = SE->getTypeSizeInBits(AS->getType());
    APInt ASizeInt(BitWidth, LocA.Size);
    APInt BSizeInt(BitWidth, LocB.Size);

    // Compute the difference between the two pointers.
    const SCEV *BA = SE->getMinusSCEV(BS, AS);

    // Test whether the difference is known to be great enough that memory of
    // the given sizes don't overlap. This assumes that ASizeInt and BSizeInt
    // are non-zero, which is special-cased above.
    if (ASizeInt.ule(SE->getUnsignedRange(BA).getUnsignedMin()) &&
        (-BSizeInt).uge(SE->getUnsignedRange(BA).getUnsignedMax()))
      return NoAlias;

    // Folding the subtraction while preserving range information can be tricky
    // (because of INT_MIN, etc.); if the prior test failed, swap AS and BS
    // and try again to see if things fold better that way.

    // Compute the difference between the two pointers.
    const SCEV *AB = SE->getMinusSCEV(AS, BS);

    // Test whether the difference is known to be great enough that memory of
    // the given sizes don't overlap. This assumes that ASizeInt and BSizeInt
    // are non-zero, which is special-cased above.
    if (BSizeInt.ule(SE->getUnsignedRange(AB).getUnsignedMin()) &&
        (-ASizeInt).uge(SE->getUnsignedRange(AB).getUnsignedMax()))
      return NoAlias;
  }

  // If ScalarEvolution can find an underlying object, form a new query.
  // The correctness of this depends on ScalarEvolution not recognizing
  // inttoptr and ptrtoint operators.
  Value *AO = GetBaseValue(AS);
  Value *BO = GetBaseValue(BS);
  if ((AO && AO != LocA.Ptr) || (BO && BO != LocB.Ptr))
    if (alias(Location(AO ? AO : LocA.Ptr,
                       AO ? +UnknownSize : LocA.Size,
                       AO ? 0 : LocA.TBAATag),
              Location(BO ? BO : LocB.Ptr,
                       BO ? +UnknownSize : LocB.Size,
                       BO ? 0 : LocB.TBAATag)) == NoAlias)
      return NoAlias;

  // Forward the query to the next analysis.
  return AliasAnalysis::alias(LocA, LocB);
}