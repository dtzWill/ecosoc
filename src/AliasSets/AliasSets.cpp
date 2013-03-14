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
//#include "llvm/ADT/SmallPtrSet.h"
//#include "llvm/ADT/SmallSet.h"

#include<set>
#include<map>
#include<queue>

using namespace llvm;

namespace {
	struct AliasSets : public ModulePass {
		static char ID;
		std::map< int, std::set<Value*> > finalValueSets;
		std::map< int, std::set<int> > finalMemSets;
		AliasSets() : ModulePass(ID) {}
		virtual bool runOnModule(Module &M) {
			PADriver &PD = getAnalysis<PADriver>();
			PointerAnalysis &PA = *PD.pointerAnalysis;
			std::map<Value*, std::vector<int> > memoryBlocks = PD.memoryBlock; // map that relates memory positions to its alloca/malloc/... instructions
			std::map<int, std::set<int> > allPointsTo = PA.allPointsTo(); // sets of alias represented as ints
			std::map<int, Value*> int2value = PD.int2value; // map to "translate" from int to Value*
			std::set<int> memPositions; // contains the ints that represent memory positions
			std::set<Value*> valueSet; // auxiliar value set
			std::set<int> intSet; //auxiliar int set
			std::vector<int> aux; // auxiliar int set
			std::map<int, std::set<int> > pointedBy; // the transposed allPointTo graph
//			PA.print();
			// Discovering which ints represent mem positions
			for (std::map<Value*, std::vector<int> >::iterator i = memoryBlocks.begin(), e = memoryBlocks.end(); i != e; ++i) {
				aux = i->second;
				for (std::vector<int>::iterator ii = aux.begin(), ee = aux.end(); ii != ee; ++ii) {
					memPositions.insert(*ii);
				}
			}
			// Building transposed graph pointedBy
			int vert;
			int pointer;
			std::set<int> adj;
			for (std::map<int, std::set<int> >::iterator i = allPointsTo.begin(), e = allPointsTo.end(); i != e; ++i) {
				vert = i->first; // vertex we will be looking for in adj sets
				pointedBy[vert].clear();
				for (std::map<int, std::set<int> >::iterator ii = allPointsTo.begin(), ee = allPointsTo.end(); ii != ee; ++ii) {
					adj = ii->second;
					if(adj.count(vert)) {
					pointer = ii->first;
						pointedBy[vert].insert(pointer);
					}
				}
			}
			// Reversed search through graph to find alias sets
			
			int memPos;
			std::map< int, std::set<int> > intAliasSets; // alias sets still represented by ints
			std::set<int> aliasSet; // auxiliar alias set to build the set above
			int v;
			for (std::set<int>::iterator i = memPositions.begin(), e = memPositions.end(); i != e; ++i) { //for each mem position
				memPos = *i;
				aliasSet.clear();
				std::queue<int> q;
				q.push(memPos);
				aliasSet.insert(memPos);
				while (!q.empty()) {
					v = q.front();
					q.pop();
					for (std::set<int>::iterator ii = pointedBy[v].begin(), ee = pointedBy[v].end(); ii != ee; ++ii) {
						if (!aliasSet.count(*ii)) // if adj vertex is not in alias set
							aliasSet.insert(*ii); //add it
							q.push(*ii);
					}
				}
				intAliasSets[memPos] = aliasSet;
			}
			// Merge non-disjoint sets

			std::set<int> union_set; //aux set to perform union
			std::set<int> to_erase; //cannot erase sets that were merged inside loop, so use this flag to mark sets that should've been erased
			// Merging non-disjoint sets
			bool changed = true;
			while (changed) {
				changed = false;
				for (std::map< int, std::set<int> >::iterator i = intAliasSets.begin(), e = intAliasSets.end(); i != e; ++i) { //for each set s
					if (!to_erase.count(i->first)) {
						for (std::set<int>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ++ii) { // for each e in s
							for (std::map< int, std::set<int> >::iterator iii = intAliasSets.begin(), eee = intAliasSets.end(); iii != eee; ++iii) { // for each set s'
								if (!to_erase.count(iii->first) && i->first < iii->first && iii->second.count(*ii)) { // if s != s' and e in s'
									// merge s and s'
									changed = true;
									std::set_union(i->second.begin(), i->second.end(), iii->second.begin(), iii->second.end(), std::inserter(union_set, union_set.end()));
									intAliasSets[i->first] = union_set;
//									errs() << "Merging " << i->first << " and " << iii->first << "\n";
									to_erase.insert(iii->first);
								}
							}
						}
					}
				}
			}
			for (std::set<int>::iterator i = to_erase.begin(), e = to_erase.end(); i !=e ; ++i) {
				intAliasSets.erase(*i);
			}
//			for (std::map< int, std::set<int> >::iterator i = intAliasSets.begin(), e = intAliasSets.end(); i != e; ++i) {
//				aliasSet = i->second;
//				errs() << "Set " << i->first << " :";
//				for (std::set<int>::iterator ii = aliasSet.begin(), ee = aliasSet.end(); ii != ee; ++ii) {
//					errs() << *ii << " ";
//				}
//				errs() << "\n";
//			}

			//Translate ints to Value* when applicable
			int count = 0;
			Value* val;
			for (std::map< int, std::set<int> >::iterator i = intAliasSets.begin(), e = intAliasSets.end(); i != e; ++i) {
				aliasSet = i->second;
				for (std::set<int>::iterator ii = aliasSet.begin(), ee = aliasSet.end(); ii != ee; ++ii) {
					if (int2value.count(*ii)) { //if it represents a Value*
						val = int2value[*ii];
						finalValueSets[count].insert(val);
					}
					else { // it represents a memory position
						finalMemSets[count].insert(*ii);
					}
				}
				++count;
			}
			DEBUG(printSets());
			return false;
		}
		
		void printSets() {
			int key;
			for (std::map< int, std::set<int> >::iterator i = finalMemSets.begin(), e = finalMemSets.end();e != i; ++i) {
				key = i->first;
				errs() << "Alias set " << key << ": ";
				for (std::set<int>::iterator ii = i->second.begin(), ee = i->second.end(); ii != ee; ++ii) {
					errs() << "m" << *ii << " ";
				}
				for (std::set<Value*>::iterator ii = finalValueSets[key].begin(), ee = finalValueSets[key].end(); ii != ee; ++ii) {
					errs() << *ii << " "; 
				}
				errs() << "\n";
			}
		}
		
		virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.addRequired<PADriver>();
			AU.setPreservesAll();
		}
		std::map< int, std::set<Value*> > getValueSets() {
			return finalValueSets;
		}
		std::map< int, std::set<int> > getMemSets() {
			return finalMemSets;
		}
	};
}


char AliasSets::ID = 0;
static RegisterPass<AliasSets> X("alias-sets", "Get alias sets from pointer analysis pass", false, false);