#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/ADT/ilist.h"
#include "stdlib.h"
#include <set>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include <list>
#include <algorithm>
#include <iterator>
#include "ArrayBoundsCheckPass.h"
#include "AvailableAndVeryBusyCheckAnalysis.h"

using namespace llvm;

char AvailableAndVeryBusyCheckAnalysis::ID = 0;
static RegisterPass<AvailableAndVeryBusyCheckAnalysis> E("a-vb-analysis", "Available and Very Busy Checks Analysis", false, false);

ValuesSet* SetUnion(ValuesSet* S1, ValuesSet* S2)
{
	ValuesSet* unionSet = new ValuesSet();
	std::set_union(S1->begin(), S1->end(), S2->begin(), S2->end(), std::inserter(*unionSet, unionSet->end()));
	return unionSet;
}

ValuesSet* SetIntersection(ValuesSet* S1, ValuesSet* S2)
{
	ValuesSet* intersectSet = new ValuesSet();
	std::set_intersection(S1->begin(), S1->end(), S2->begin(), S2->end(), std::inserter(*intersectSet, intersectSet->end()));
	return intersectSet;
}

ValuesSet* SetsMeet(ListOfValuesSets* sets, ValuesSet*(*meet)(ValuesSet*, ValuesSet*))
{
	ValuesSet* lastSet = new ValuesSet();
	for(std::list<ValuesSet*>::iterator II = sets->begin(), IE = sets->end(); II != IE; II++){
		ValuesSet* currentSet = meet(lastSet, *II);
		//this may be a little slow but I am just trying to finish this
		delete lastSet;
		lastSet = currentSet;
	}
	return lastSet;
}

bool SetEqual(ValuesSet* S1, ValuesSet* S2)
{
	if(S1->size() > S2->size())
		return std::equal(S1->begin(), S1->end(), S2->begin());
	else
		return std::equal(S2->begin(), S2->end(), S1->begin());
}

ValuesSet* forward(ValuesSet* C_IN, BasicBlock* BB)
{
	return C_IN;
}

ValuesSet* backward(ValuesSet* C_OUT, BasicBlock* BB)
{
	return C_OUT;
}

bool AvailableAndVeryBusyCheckAnalysis::doInitialization(Module& M)
{
	// stub function. do not delete. keeps the compiler warnings and errors at bay
	return false;
}

bool AvailableAndVeryBusyCheckAnalysis::runOnFunction(Function& F)
{
	//	
	this->currentFunction = &F;
	this->AA = &this->getAnalysis<AliasAnalysis>();
	this->SE = &this->getAnalysis<ScalarEvolution>();
	this->findGenSets();
	return true;
}


///we should probably keep a map of the affect type for a given variable, to make this faster if we already
///check a variable previously
AvailableAndVeryBusyCheckAnalysis::EffectTy AvailableAndVeryBusyCheckAnalysis::effect(BasicBlock* B, Value* v)
{
	return changedTy;
}

void AvailableAndVeryBusyCheckAnalysis::dataFlowAnalysis(bool isForward)
{
	if(!isForward)
	{
		MapBBToValuesSet* IN = new MapBBToValuesSet();
///		MapBBToValuesSet* OUT = new MapBBToValuesSet();
		ListOfValuesSets ALL;
		for(MapBBToValuesSet::iterator II = this->VeryBusy_Gen->begin(), IE = this->VeryBusy_Gen->end(); II != IE; II++) {ALL.push_back(II->second);}
		///universal set
		ValuesSet* U = SetsMeet(&ALL, &SetUnion);
		ValuesSet* N = new ValuesSet();
		///initialize each basic block information
		for(MapBBToValuesSet::iterator II = this->VeryBusy_Gen->begin(), IE = this->VeryBusy_Gen->end(); II != IE; II++){
			BasicBlock* BB = II->first;
			IN->insert(PairBBToValuesSet(BB, U));
		}
		bool inChanged = true;
		int i = 0;
		while(inChanged){
			inChanged = false;
			///go throught each basic block
			errs() << "^^^^^^^^^^^^^^RUN " << i << "^^^^^^^^^^^^^^\n";
			for(MapBBToValuesSet::iterator II = this->VeryBusy_Gen->begin(), IE = this->VeryBusy_Gen->end(); II != IE; II++){
				BasicBlock* BB = II->first;
				ValuesSet* C_GEN = II->second;
				//successor IN sets
				ListOfValuesSets S_INS;
				//for every sucessor block
				errs() << BB->getName() << "\n";
				for(BasicBlock::use_iterator SBBI = BB->use_begin(), SBBE = BB->use_end(); SBBI != SBBE; SBBI++) { 
					BasicBlock* SBB = dyn_cast<BasicBlock>(*SBBI);
					ValuesSet* SBB_IN = (*IN)[SBB];
					S_INS.push_back(SBB_IN);
				}
					
				ValuesSet* C_OUT = SetsMeet(&S_INS, &SetIntersection);
				ValuesSet* BB_IN = (*IN)[BB];
				//transition C_OUT to IN
				ValuesSet* T = backward(C_OUT, BB);
				ValuesSet* BB_N_IN = SetIntersection(C_GEN, T);
				///store new basic block IN set
				IN->erase(BB);
				IN->insert(PairBBToValuesSet(BB, BB_N_IN));
				///check if IN changed
				if(!SetEqual(BB_IN, BB_N_IN))
					inChanged = true;
				///clean up unneeded memory
				delete C_OUT;
				delete BB_IN;
			}
			errs() << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n";
			i++;
		}
		delete U;
		delete N;
		delete IN;
	}
	else
	{
		//to be done	
	}
}



void AvailableAndVeryBusyCheckAnalysis::findGenSets()
{
	///
	///both the very busy, and available gen sets are calculated
	///for every basic block
	///
	this->VeryBusy_Gen = new MapBBToValuesSet();
	this->Available_Gen = new MapBBToValuesSet();

	for(llvm::Function::iterator IBB = this->currentFunction->begin(), EBB = this->currentFunction->end(); IBB != EBB; IBB++)
	{
		BasicBlock& BB = *IBB;
		RangeCheckSet* currentRCS = new RangeCheckSet
		///go throught each block
		for(llvm::BasicBlock::iterator II = BB.begin(), EI = BB.end(); II != EI; II++)
		{
			if(CallInst* callInst = dyn_cast<CallInst>(&*II)){
				const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
				if(!callFunctionName.equals("checkLTLimit") && !callFunctionName.equals("checkGTZero")){
					continue;
				}
				BBVB_IN->insert(callInst);
			}
			else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II))
			{

			}
		}
		///
		///sets generated are stored for each block
		///
		VeryBusy_Gen->insert(PairBBToValuesSet(&BB, BBVB_IN));
		Available_Gen->insert(PairBBToValuesSet(&BB, BBVB_IN));
		//this->dumpSetOfPtr(BBVB_IN);
	}
	/*
	ValuesSet* lastSet = NULL;
	for(MapBBToValuesSet::iterator II = VeryBusy_Gen->begin(), IE = VeryBusy_Gen->end(); II != IE; II++)
	{
		ValuesSet* currentSet = II->second;
		if(lastSet)
		{	
			ValuesSet* intSet = SetUnion(currentSet, lastSet);
			errs() << "--------------set union---------------\n";
			errs() << "set 1 - " << SetEqual(currentSet, intSet) << "\n";
			this->dumpSetOfPtr(currentSet);
			errs() << "set 2 - " << SetEqual(lastSet, intSet) << "\n";
			this->dumpSetOfPtr(lastSet);
			errs() << "set out - " << SetEqual(intSet, intSet) << "\n";
			this->dumpSetOfPtr(intSet);
			errs() << "--------------------------------------\n";
			delete intSet;
		}
		lastSet = currentSet;
	}
	*/
}



template <typename T> void AvailableAndVeryBusyCheckAnalysis::dumpSetOfPtr(std::set<T*> *set)
{
	llvm::errs() << "{\n";
	for(typename std::set<T*>::iterator IP = set->begin(), EP = set->end(); IP != EP; IP++)
	{
		llvm::errs() << "\t" << **IP << "\n";
	}
	if(set->empty())
	{
		llvm::errs() << "\tempty" << "\n";
	}
	llvm::errs() << "}\n";
}

bool AvailableAndVeryBusyCheckAnalysis::doFinalization(Module& M)
{
	return false;	
}
