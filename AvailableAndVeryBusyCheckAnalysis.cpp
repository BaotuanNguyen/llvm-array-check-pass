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
void AvailableAndVeryBusyCheckAnalysis::findGenSets()
{
	///
	///both the very busy, and available gen sets are calculated
	///for every basic block
	///
	MapBBToValuesSet* VeryBusy_Gen = new MapBBToValuesSet();
	MapBBToValuesSet* Available_Gen = new MapBBToValuesSet();

	for(llvm::Function::iterator IBB = this->currentFunction->begin(), EBB = this->currentFunction->end(); IBB != EBB; IBB++)
	{
		BasicBlock& BB = *IBB;
		ValuesSet* BBVB_IN = new ValuesSet();
		//find gen of the block
		for(llvm::BasicBlock::iterator II = BB.begin(), EI = BB.end(); II != EI; II++)
		{
			if(CallInst* callInst = dyn_cast<CallInst>(&*II))
			{
				//should only be inserted if it is a gen call check instruction
				const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
				///
				///check whether it is a check call
				///
				if(!callFunctionName.equals("checkLTLimit") && !callFunctionName.equals("checkGTZero"))
				{
					continue;
				}
				///
				///insert into values set
				///
				BBVB_IN->insert(callInst);
			}
		}
		///
		///sets generated are stored for each block
		///
		VeryBusy_Gen->insert(PairBBToValuesSet(&BB, BBVB_IN));
		Available_Gen->insert(PairBBToValuesSet(&BB, BBVB_IN));
		//this->dumpSetOfPtr(BBVB_IN);
	}
	ValuesSet* lastSet = NULL;
	/*
	for(MapBBToValuesSet::iterator II = VeryBusy_Gen->begin(), IE = VeryBusy_Gen->end(); II != IE; II++)
	{
		ValuesSet* currentSet = II->second;
		if(lastSet)
		{	
			errs() << "--------------set union---------------\n";
			errs() << "set 1 -\n";
			this->dumpSetOfPtr(currentSet);
			errs() << "set 2 -\n";
			this->dumpSetOfPtr(lastSet);
			ValuesSet* intSet = SetIntersection(currentSet, lastSet);
			errs() << "set out -\n";
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
