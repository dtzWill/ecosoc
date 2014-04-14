#define DEBUG_TYPE "ranged-alias-sets"
#include "RangedAliasSets.h"

using namespace llvm;

/*
* Statistics macros and global variables
*/

// These macros are used to get stats regarding the precision of our analysis.
STATISTIC(NAliasSets, "Number of original alias sets");
STATISTIC(NInterestingSets, "Number of alias sets that were divided");
STATISTIC(NRangedSets, "Number of ranged alias sets found");
STATISTIC(NNewSets, "Number of alias sets found from the divided");
STATISTIC(NFinalSets, "Number of final alias sets");
//Global Variables Declarations
extern APInt Min;
extern APInt Max;
extern APInt Zero;

/*
* MemRange Support functions
*/

//Finds the MemRange of element in a MemRange set
RangedAliasSets::MemRange* //Returns
RangedAliasSets::MemRange::FindByValue //Name
(Value* element, std::set<MemRange*> MRSet) //Parameters
{
	for(std::set<MemRange*>::iterator i = MRSet.begin(), e = MRSet.end(); 
	i != e;	i++)
		if(element == (*i)->mem)
			return *i;
	
	return NULL;
}

/*
* RangedAliasSets Support functions
*/

//Verifies if the instruction verify is present in vector arr
bool //Returns
RangedAliasSets::isPresent //Name
(Instruction* verify, std::vector<Instruction*> arr) //Parameters
{
	int i;
	for(std::vector<Instruction*>::iterator it = arr.begin() ; 
	it != arr.end(); it++)
		if(verify == *it) return true;
	
	return false;
}

//Takes a vector of instructions (ptr) and orders them
std::vector<Instruction*> //Returns
RangedAliasSets::orderInstructions //name
(std::vector<Instruction*> unordered, Module* M) //Parameters
{
	std::vector<Instruction*> ordered(unordered.size());
	int i = 0;
	for (Module::iterator F = M->begin(), Fe = M->end(); F != Fe; F++)
		for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
			if(isPresent(&(*I), unordered)){
				ordered[i] = &(*I);
				i++;
			}
	
	return ordered;
}

//methods that return the persistent maps
llvm::DenseMap<int, std::set<RangedAliasSets::MemRange*> > //Returns
RangedAliasSets::getRangedAliasSets //Name
() //No parameters
{
	return RangeAliasSets;
}

llvm::DenseMap<int, std::set<Value*> > //Returns 
RangedAliasSets::getAliasSets //Name
() //No parameters
{
	return NewAliasSets;
}

/*
* Debug Functions
*/

void //Returns Nothing 
RangedAliasSets::printRangeAnalysis //Name
(InterProceduralRA<Cousot> *ra, Module *M) //Parameters
{
	errs() << "Cousot Inter Procedural analysis (Values -> Ranges):\n";
	for(Module::iterator F = M->begin(), Fe = M->end(); F != Fe; ++F)
		for(Function::iterator bb = F->begin(), bbEnd = F->end(); bb != bbEnd; ++bb)
			for(BasicBlock::iterator I = bb->begin(), IEnd = bb->end(); 
			I != IEnd; ++I){
				const Value *v = &(*I);
				Range r = ra->getRange(v);
				if(!r.isUnknown()){
					r.print(errs());
					I->dump();
				}
			}
			
		errs() << "--------------------\n";
}

void //Returns Nothing 
RangedAliasSets::printAliasSets //Name
(llvm::DenseMap<int, std::set<Value*> > *AliasSets) //Parameters
{
	errs() << "Alias Sets:" << "\n";
	for (llvm::DenseMap<int, std::set<Value*> >::iterator i = AliasSets->begin(), e = AliasSets->end(); 
	i != e; ++i) {
		  errs() << "Set " << i->first << " : size? "<< i->second.size() <<"\n";
        for (std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); 
        ii != ee; ++ii) {

            errs() << "	" << **ii <<"  inst? " << isa<Instruction>(**ii) << "\n";
        }

        errs() << "\n";
	}
	
	errs() << "--------------------\n";
}

void //Returns Nothing 
RangedAliasSets::printInterestingSets //Name
(llvm::DenseMap<int, std::set<Value*> > *InterestingSets) //Parameters
{
	errs() << "Insteresting Sets:" << "\n";	
	for (llvm::DenseMap<int, std::set<Value*> >::iterator i = InterestingSets->begin(), e = InterestingSets->end(); 
	i != e; ++i) {
        errs() << "Set " << i->first << "\n";
        for (std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); 
        ii != ee; ++ii) {

            errs() << "	" << **ii << "\n";
        }

        errs() << "\n";
	}
	
	errs() << "--------------------\n";
}

void //Returns Nothing 
RangedAliasSets::printInterestingVectors //Name
(llvm::DenseMap<int, std::vector<Instruction*> > *InterestingVectors) //Parameters
{
	errs() << "Insteresting Vectors" << "\n";
	for (llvm::DenseMap<int, std::vector<Instruction*> >::iterator i = InterestingVectors->begin(), e = InterestingVectors->end(); 
	i != e; ++i) {
        errs() << "Vector " << i->first << ":\n";
        for (std::vector<Instruction*>::iterator ii = i->second.begin(), ee = i->second.end(); 
        ii != ee; ++ii) {

            errs() << **ii << "\n";
        }

        errs() << "\n";
	}
	
	errs() << "--------------------\n";
}

void //Returns Nothing 
RangedAliasSets::printMemRanges //Name
(llvm::DenseMap<int, std::set<MemRange*> > *MemRangeSets) //Parameters
{
	errs() << "Memory Ranges" << "\n";
	for (llvm::DenseMap<int, std::set<MemRange*> >::iterator i = MemRangeSets->begin(), e = MemRangeSets->end(); 
	i != e; ++i){
		errs() << "Set " << i->first << ":\n";
		for (std::set<MemRange*>::iterator ii = i->second.begin(), ee = i->second.end(); 
      ii != ee; ++ii){
      	errs() << *((*ii)->mem) << "   ->   ";
      	errs() << " [" << (*ii)->lower << "," << (*ii)->higher << "]\n";
      }
	}
	
	errs() << "--------------------\n";
	
	//This commented code is here for it's usefulness and because i'll be using parts of it in the future
	/*for (llvm::DenseMap<int, std::set<MemRange*> >::iterator i = MemRangeSets->begin(), e = MemRangeSets->end(); 
	i != e; ++i)
	{
		for (std::set<MemRange*>::iterator ii = i->second.begin(), ee = i->second.end(); 
    ii != ee; ++ii)
    {
      	if(isa<GetElementPtrInst>(*((*ii)->mem))){
			 		//Mem Range basePtrMemRange + indexes range
       		Value* base_ptr = ((GetElementPtrInst*)((*ii)->mem))->getPointerOperand();
      		errs() << *(base_ptr) << "\n";
      		errs() << *(base_ptr->getType()) << "\n";
      		Type* x = base_ptr->getType();
      		if(x->isPointerTy())
      		{
      			x = x->getPointerElementType();
      			if(x->isArrayTy())
      				errs() << x->getArrayNumElements() << "ARRAY\n";
      		}
      			
      	}
      	
	  }
	}
	errs() << "--------------------\n";*/
}

void //Returns Nothing 
RangedAliasSets::printRangeAliasSets //Name
(llvm::DenseMap<int, std::set<MemRange*> > *RangedAliasSets) //Parameters
{
	errs() << "\n-------------------------\nRANGED ALIAS SETS:" << "\n";
	
	for(int i = 1; i <= RangedAliasSets->size(); i++){
		errs() << "Set " << i << ":\n";
		for (std::set<MemRange*>::iterator ii = (*RangedAliasSets)[i].begin(), ee = (*RangedAliasSets)[i].end(); 
      ii != ee; ++ii){
      	errs() << *((*ii)->mem) << "   ->   ";
      	errs() << " [" << (*ii)->lower << "," << (*ii)->higher << "]\n";
      }
	}
	
	errs() << "-------------------------\n\n";
}

void //Returns Nothing 
RangedAliasSets::printNewAliasSets //Name
(llvm::DenseMap<int, std::set<Value*> > *NewAliasSets) //Parameters
{
	errs() << "\n-------------------------\nNEW ALIAS SETS:" << "\n";
	for(int i = 1; i <= NewAliasSets->size(); i++){
		errs() << "Set " << i << ":\n";
		for (std::set<Value*>::iterator ii = (*NewAliasSets)[i].begin(), ee = (*NewAliasSets)[i].end(); 
      ii != ee; ++ii){
      	errs() << **ii << "\n";
      }
	}
	errs() << "-------------------------\n\n";
}

/*
*
* LLVM framework Main Function
*		Does the main analysis
*
*/

bool //Returns 
RangedAliasSets::runOnModule //Name
(Module &M) //Parameters
{

	/*
	* Gets RangeAnalysis Pass Data.
	*/
	
	InterProceduralRA<Cousot> &ra = getAnalysis<InterProceduralRA<Cousot> >();
	unsigned MaxBitWidth = InterProceduralRA<Cousot>::getMaxBitWidth(M);
	DEBUG(printRangeAnalysis(&ra, &M));
	
	/*
	* Gets AliasSets Pass Date.
	*/
	
	AliasSets &AS = getAnalysis<AliasSets>();
	llvm::DenseMap<int, std::set<Value*> > AliasSets = AS.getValueSets();
	NAliasSets = 0;//statistics
	for (llvm::DenseMap<int, std::set<Value*> >::iterator i = AliasSets.begin(), e = AliasSets.end(); 
	i != e; ++i) if(i->second.size() > 0) NAliasSets++;
	DEBUG(printAliasSets(&AliasSets));
		
	/*
	* Calculate Structs Primitive Layouts.
	*/
	
	//????
	
	/*
	* Selects Interesting Sets, wich are the ones the pass will try do
	* divide. These sets must have just one memory base pointer, more than one
	* element, all it's elements must be Instructions	.
	*/	
	
	llvm::DenseMap<int, std::set<Value*> > InterestingSets;
	int set_number = 0;
	
	//Checking for apropriate sets for each alias set
	for (llvm::DenseMap<int, std::set<Value*> >::iterator i = AliasSets.begin(), 
	e = AliasSets.end(); i != e; ++i){
		//if size of set less or equal to 1 element, not interesting set
		if(i->second.size() <= 1) continue;
		//there can only be one general base ptr and all instructions
		int alloca_count = 0;
		bool non_valid = false;
		//foreach element of the set
		for (std::set<Value*>::iterator ii = i->second.begin(), 
		ee = i->second.end(); ii != ee; ++ii){
			//if element is no an Instruction
			if(!isa<Instruction>(**ii)){
        			non_valid = true;
        			break;
      }
      
      //checking element for types of instructions 
      if(isa<AllocaInst>(**ii))
      	alloca_count++;
      else if(isa<CallInst>(**ii)){
      	if( strcmp( ((CallInst*)*ii)->getCalledFunction()->getName().data(), "malloc") == 0 
      	or strcmp( ((CallInst*)*ii)->getCalledFunction()->getName().data(), "calloc") == 0
      	or strcmp( ((CallInst*)*ii)->getCalledFunction()->getName().data(), "realloc") == 0	)
        	alloca_count++;
			}
			else if(isa<LoadInst>(**ii))
				alloca_count++;
		}
		//if all rules are valid, this is an interesting set
		if(alloca_count == 1 and non_valid == false){
    	set_number++;
    	InterestingSets[set_number] = i->second;
    }
	}
	DEBUG(printInterestingSets(&InterestingSets));
	
	/*
	* Takes all Interesting Sets of Value* and 
	* orders them in vectors of Instruction*.
	*/
	llvm::DenseMap<int, std::vector<Instruction*> > InterestingVectors;
	int InterestingVectors_i = 1;
	
	for (llvm::DenseMap<int, std::set<Value*> >::iterator i = InterestingSets.begin(), 
	e = InterestingSets.end(); i != e; ++i){
		std::vector<Instruction*> unordered(i->second.size());
		int vector_i = 0;
		for (std::set<Value*>::iterator ii = i->second.begin(), ee = i->second.end(); 
      ii != ee; ++ii){ 
      	unordered[vector_i] = (Instruction*) *ii;
      	vector_i++;
      }
      InterestingVectors[InterestingVectors_i] = orderInstructions(unordered, &M); 
      InterestingVectors_i++;
	}
	DEBUG(printInterestingVectors(&InterestingVectors));
	
	
	/*
	* Calculates MemoryRanges for each Interesting Vector and each
	* instruction element in them.
	*/
	
	//llvm::DenseMap<int, std::set<MemRange*> > MemRangeSets;
	int MemRangeSets_i = 1;
	//For each interesting vector
	for (llvm::DenseMap<int, std::vector<Instruction*> >::iterator i = InterestingVectors.begin(), 
	e = InterestingVectors.end(); i != e; ++i)
	{
		bool error = false;
		std::set<MemRange*> memSet;
		Value * base_aloc;
		Type * base_type;
		//For each instruction in the current interesting vector
		for (std::vector<Instruction*>::iterator ii = i->second.begin(), 
		ee = i->second.end(); ii != ee; ++ii)
		{
			//Tries to indetify the instruction and do proper calculations
     	if(isa<AllocaInst>(**ii))
     	{
	   		//Mem Range [0,0]
	   		base_aloc = *ii;
	   		base_type = base_aloc->getType();
     		memSet.insert(new MemRange(*ii, Zero, Zero, base_aloc));
      }
			else if(isa<CallInst>(**ii))
			{
   			if( strcmp( ((CallInst*)*ii)->getCalledFunction()->getName().data(), "malloc") == 0 
   			or strcmp( ((CallInst*)*ii)->getCalledFunction()->getName().data(), "calloc") == 0
   			or strcmp( ((CallInst*)*ii)->getCalledFunction()->getName().data(), "realloc") == 0	)
   			{
	 				base_aloc = *ii;
	   			base_type = base_aloc->getType();
	 				memSet.insert(new MemRange(*ii, Zero, Zero, base_aloc));
				}
			}
			else if(isa<GetElementPtrInst>(**ii))
			{
      	//Mem Range basePtrMemRange + indexes range
       	Value* base_ptr = ((GetElementPtrInst*)*ii)->getPointerOperand();
       	MemRange* base_range = MemRange::FindByValue(base_ptr, memSet);
       	
       	/*
       	* Superficial analysis implementation, if base_ptr isn't of the same
       	* type as the alocation ptr than it means this pointer is looking 
       	* inside a struct and thus its memory range will be the same as
       	* it's base_ptr
       	*/ 
       	if(base_ptr->getType() != base_type)
       	{
       		memSet.insert(new MemRange(*ii,base_range->lower,base_range->higher, base_aloc));
       		continue;
       	}
       	
       	//Summing up all indexes
       	APInt lower_range;
       	APInt higher_range;
       	lower_range = base_range->lower;
       	higher_range = base_range->higher;
       	for(User::op_iterator idx = ((GetElementPtrInst*)*ii)->idx_begin(), 
       	idxe = ((GetElementPtrInst*)*ii)->idx_end(); idx != idxe; idx++)
       	{
       		Value* indx = idx->get();
         	if(isa<ConstantInt>(*indx))
         	{
         		lower_range = (lower_range == Min) ? Min : 
         		(lower_range + (((ConstantInt*)indx)->getValue()).sextOrTrunc(lower_range.getBitWidth()) );
         		
         		higher_range = (higher_range == Max) ? Max : 
         		(higher_range + (((ConstantInt*)indx)->getValue()).sextOrTrunc(higher_range.getBitWidth()) );
         	}
         	else
         	{
         		Range r = ra.getRange(indx);
           	if(!r.isUnknown())
           	{
            	lower_range = (lower_range == Min || r.getLower() == Min) ? Min : 
            	(lower_range + r.getLower()); 
         			
         			higher_range = (higher_range == Max || r.getUpper() == Max) ? Max : 
         			(higher_range + r.getUpper());
      	    }
         		
       		}
       	}
         	
      	memSet.insert(new MemRange(*ii,lower_range,higher_range, base_aloc));
			}
      else if(isa<BitCastInst>(**ii))
      {
       	Value* base_ptr = (*ii)->getOperand(0);
       	MemRange* base_range = MemRange::FindByValue(base_ptr, memSet);
       	memSet.insert(new MemRange(*ii,base_range->lower,base_range->higher, base_aloc));
      }
      else //Any other instruction
      {
      	error = true;
      	break;
      }
  	}
  	if(!error)
  	{
    MemRangeSets[MemRangeSets_i] = memSet;
    MemRangeSets_i++;
		}
	}
	
	DEBUG(printMemRanges(&MemRangeSets));
	
	/*
	* Building Ranges Alias Sets
	*/
	
	//llvm::DenseMap<int, std::set<MemRange*> > RangeAliasSets;
	int RangeAliasSets_i = 1;
	for (llvm::DenseMap<int, std::set<MemRange*> >::iterator i = MemRangeSets.begin(), e = MemRangeSets.end(); 
	i != e; ++i)
	{
		APInt MinusOne = Zero;
		MinusOne--;
		APInt One = Zero;
		One++;
		APInt lower_range = MinusOne;
		APInt higher_range = MinusOne;
		while(true)
		{
			std::set<MemRange*> rangedMemSet;
			//Calculating new lower range
			APInt new_lower_range = Max;
			for (std::set<MemRange*>::iterator ii = i->second.begin(), ee = i->second.end(); 
	    ii != ee; ++ii)
	    {
	     	if((*ii)->higher.sle(higher_range)){
	     		i->second.erase(*ii);
	     		continue;
	     	}
	     	else{
	     		if((*ii)->lower.sle(higher_range)) new_lower_range = higher_range + One;
	     		else if((*ii)->lower.slt(new_lower_range)) new_lower_range = (*ii)->lower;
	     	}
	    }
	    lower_range = new_lower_range;
	    //Calculating new higher range
	    APInt new_higher_range1 = Max;
			APInt new_higher_range2 = Max;
			for (std::set<MemRange*>::iterator ii = i->second.begin(), ee = i->second.end(); 
	    ii != ee; ++ii)
	    {
	     	if((*ii)->higher.slt(new_higher_range1))new_higher_range1 = (*ii)->higher;
	      	
	     	if((*ii)->lower.sgt(lower_range) and (*ii)->lower.slt(new_higher_range2)) 
	     		new_higher_range2 = (*ii)->lower - One;
	    }
	    higher_range = new_higher_range1.slt(new_higher_range2) ? new_higher_range1 : new_higher_range2;
	    if(i->second.empty()) break;
	    //Add members
			for (std::set<MemRange*>::iterator ii = i->second.begin(), ee = i->second.end(); 
      ii != ee; ++ii)
      {
      	if((*ii)->lower.sle(higher_range))
      	{
      		rangedMemSet.insert(new MemRange((*ii)->mem, lower_range, higher_range,(*ii)->aloc) );		
      	}
      }
      RangeAliasSets[RangeAliasSets_i] = rangedMemSet;
      RangeAliasSets_i++;
    }
 	}
 	NRangedSets = RangeAliasSets.size();//statistics
	DEBUG(printRangeAliasSets(&RangeAliasSets));
	
	/*
	* Building New Alias Sets, now more precise
	*/
	
	////Merging appropriate table cells
	//llvm::DenseMap<int, std::set<Value*> > NewAliasSets;
	int newi = 1;
	for (std::set<MemRange*>::iterator ii = RangeAliasSets[1].begin(), ee = RangeAliasSets[1].end(); 
  ii != ee; ++ii)
    NewAliasSets[newi].insert((*ii)->mem);
      
  for(int i = 2; i <= RangeAliasSets.size(); i++)
  {
		bool merge = false;
		for (std::set<MemRange*>::iterator ii = RangeAliasSets[i].begin(), ee = RangeAliasSets[i].end(); 
    ii != ee; ++ii)
    {
     	if( NewAliasSets[newi].count((*ii)->mem) )
     	{
     		merge = true;
     		break;
     	}
    }
      
    if(!merge) newi++;
      
    for (std::set<MemRange*>::iterator ii = RangeAliasSets[i].begin(), ee = RangeAliasSets[i].end(); 
    ii != ee; ++ii)
    	NewAliasSets[newi].insert((*ii)->mem);
	}
	NNewSets = NewAliasSets.size();//statistics
	////adding undivided sets
	for(int i = 1; i <= InterestingSets.size(); i++)
	{
		for(int j = 1; j <= AliasSets.size(); j++)
		{
			if(InterestingSets[i] == AliasSets[j])
				AliasSets.erase(j);
		}
	}
	
	for(int i = 1; i <= AliasSets.size(); i++)
	{
		if(!AliasSets[i].empty())
		{
			newi++;
			NewAliasSets[newi] = AliasSets[i];
		}
	}
	NFinalSets = NewAliasSets.size();//statistics
	DEBUG(printNewAliasSets(&NewAliasSets));

	/*
	* Done, end of analysis
	*/
}

/*
* LLVM framework support functions
*/

void //Returns nothing
RangedAliasSets::getAnalysisUsage //Name
(AnalysisUsage &AU) const //Parameters
{
	AU.setPreservesAll();
	AU.addRequired<AliasSets>();
	AU.addRequired<InterProceduralRA<Cousot> >();
}
char RangedAliasSets::ID = 0;
static RegisterPass<RangedAliasSets> X("ranged-alias-sets",
"Get alias sets with memory range from pointer analysis pass", false, false);