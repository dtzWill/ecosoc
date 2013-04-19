#define DEBUG_TYPE "AliasSets"
#include<AliasSets.h>

using namespace llvm;

AliasSets::AliasSets() :
	ModulePass(ID) {
}

void AliasSets::mergeSets(int i, int j) {
	std::set<int> aux;
	// Merge alias sets into 1st set
	for (std::set<int>::iterator it = intSets[j].begin(), e = intSets[j].end(); it
			!= e; ++it) {
		intSets[i].insert(*it);
	}
	//Erase 2nd set of alias sets
	intSets.erase(j);
	// Merge points-to of sets into 1st set
	for (std::set<int>::iterator it = intPointsTo[j].begin(), e =
			intPointsTo[j].end(); it != e; ++it) {
		intPointsTo[i].insert(*it);
	}
	//Erase 2nd set of points-to
	intPointsTo.erase(j);
}

bool AliasSets::analyzeSets() {
	std::map<int, std::set<int> >::iterator aux1;
	for (std::map<int, std::set<int> >::iterator i = intSets.begin(), e =
			intSets.end(); i != e; ++i) { //for each set s
		for (std::set<int>::iterator ii = intPointsTo[i->first].begin(), ee =
				intPointsTo[i->first].end(); ii != ee; ++ii) { // for each e in s
			std::map<int, std::set<int> >::iterator iii = i;
			iii++;
			for (std::map<int, std::set<int> >::iterator eee = e; iii != eee; ++iii) { // for each set s'
				if (i != iii && intPointsTo[iii->first].count(*ii)) { // if valid and e in s', merge s and s'
					mergeSets(i->first, iii->first);
					return true;
				}
			}
		}
	}
	return false;
}

void AliasSets::printIntSets(std::set<int> to_erase) {
	for (std::map<int, std::set<int> >::iterator i = intSets.begin(), e =
			intSets.end(); i != e; ++i) {
		if (!to_erase.count(i->first)) {
			errs() << "Set " << i->first << ": ";
			for (std::set<int>::iterator setIt = i->second.begin(), setEnd =
					i->second.end(); setIt != setEnd; ++setIt) {
				errs() << *setIt << " ";
			}
			errs() << "\n\tPoints-to: ";
			for (std::set<int>::iterator setIt = intPointsTo[i->first].begin(),
					setEnd = intPointsTo[i->first].end(); setIt != setEnd; ++setIt) {
				errs() << *setIt << " ";
			}
		} else {
			errs() << "Set " << i->first << " erased.";
		}
		errs() << "\n";
	}
}

bool AliasSets::runOnModule(Module &M) {
	PADriver &PD = getAnalysis<PADriver> ();
	PointerAnalysis* PA = PD.pointerAnalysis;
	std::map<Value*, std::vector<int> > memoryBlocks = PD.memoryBlock; // map that relates memory positions to its alloca/malloc/... instructions
	std::map<int, std::set<int> > allPointsTo = PA->allPointsTo(); // sets of alias represented as ints
	std::map<int, Value*> int2value = PD.int2value; // map to "translate" from int to Value*
	std::set<Value*> valueSet; // auxiliar value set
	std::vector<int> aux_mem;
	std::set<int> intSet; //auxiliar int set
	//	PA->print();
	// ** Initializing alias sets - Creating one alias set per pointer */
	//        int count = 0;
	int count = 1;
	std::set<int> s;
	for (std::map<int, std::set<int> >::iterator i = allPointsTo.begin(), e =
			allPointsTo.end(); i != e; ++i) {
		s.clear();
		s.insert(i->first);
		intSets[count] = s;
		intPointsTo[count] = i->second;
		++count;
	}
	bool changed;
	do {
		//errs() << "*\n";
		changed = analyzeSets();
	} while (changed);

	//Translate ints to Value* when applicable
	count = 0;
	int aux_count = 0;
	Value* val;
	std::set<int> aliasSet;
	for (std::map<int, std::set<int> >::iterator i = intSets.begin(), e =
			intSets.end(); i != e; ++i) {
		aliasSet = i->second;
		for (std::set<int>::iterator ii = aliasSet.begin(), ee = aliasSet.end(); ii
				!= ee; ++ii) {
			if (int2value.count(*ii)) { //if it represents a Value*
				val = int2value[*ii];
				finalValueSets[count].insert(val);
			} else { // it represents a memory position
				finalMemSets[count].insert(*ii);
			}
			aux_count++;
		}
		++count;
	}
	//      DEBUG(
	//              errs() << "PA output had " << allPointsTo.size() << " nodes.\n";
	//              errs() << "Alias Sets pass returns " << aux_count << " nodes.\n";
	//              );
	DEBUG(printSets(););
	return false;
}

void AliasSets::printSets() {
	int m_size;
	int v_size;
	if (finalMemSets.empty()) {
		m_size = 0;
	} else {
		m_size = finalMemSets.rbegin()->first;
	}
	if (finalValueSets.empty()) {
		v_size = 0;
	} else {
		v_size = finalValueSets.rbegin()->first;
	}
	int max = m_size > v_size ? m_size : v_size;
	for (int i = 0; i < max; i++) {
		errs() << "Alias set " << i << ":\n";
		if (finalMemSets.count(i)) {
			for (std::set<int>::iterator ii = finalMemSets[i].begin(), ee =
					finalMemSets[i].end(); ii != ee; ++ii) {
				errs() << "\tm" << *ii << "\n";
			}
		}
		if (finalValueSets.count(i)) {
			for (std::set<Value*>::iterator ii = finalValueSets[i].begin(), ee =
					finalValueSets[i].end(); ii != ee; ++ii) {
				errs() << "\t" << **ii << "\n";
			}
		}
	}
}

void AliasSets::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<PADriver> ();
	AU.setPreservesAll();
}

std::map<int, std::set<Value*> > AliasSets::getValueSets() {
	return finalValueSets;
}
std::map<int, std::set<int> > AliasSets::getMemSets() {
	return finalMemSets;
}

int AliasSets::getValueSetKey(Value* v) {
	for (std::map<int, std::set<Value*> >::iterator i = finalValueSets.begin(),
			e = finalValueSets.end(); i != e; ++i) {
		if (i->second.count(v))
			return i->first;
	}
	//        assert(0 && "Value requested is not in any alias set!");
	return 0;
	//        return -1;
}

int AliasSets::getMapSetKey(int m) {
	for (std::map<int, std::set<int> >::iterator i = finalMemSets.begin(), e =
			finalMemSets.end(); i != e; ++i) {
		if (i->second.count(m))
			return i->first;
	}
	//        assert(0 && "Memory positions requested is not in any alias set!");
	return 0;
	//        return -1;
}

char AliasSets::ID = 0;
static RegisterPass<AliasSets> X("alias-sets",
		"Get alias sets from pointer analysis pass", false, false);
