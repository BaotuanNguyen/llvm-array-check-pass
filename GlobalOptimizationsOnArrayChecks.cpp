#include "GlobalOptimizationsOnArrayChecks.h"
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
#include "ArrayBoundsCheckPass.h"

using namespace llvm;

char GlobalOptimizationsOnArrayChecks::ID = 0;
static RegisterPass<GlobalOptimizationsOnArrayChecks> D("global-opts", "Global optimizations on array checks performed", false, false);

bool GlobalOptimizationsOnArrayChecks::doInitialization(Module& M)
{
	// stub function. do not delete. keeps the compiler warnings and errors at bay
	return false;
}

bool GlobalOptimizationsOnArrayChecks::runOnFunction(Function& F)
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
GlobalOptimizationsOnArrayChecks::EffectTy GlobalOptimizationsOnArrayChecks::effect(BasicBlock* B, Value* v)
{
	std::list<Value*> loadStoreList;
	for(llvm::BasicBlock::iterator II = B->begin(), IE = B->end(); II != IE; II++){
		if(LoadInst* loadInst = dyn_cast<LoadInst>(&*II)){
			if(loadInst->getPointerOperand() == v){loadStoreList.push_back(loadInst);}
		}		
		else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II)){
			if(loadInst->getPointerOperand() == v){loadStoreList.push_back(storeInst);}
		}
	}
	if(loadStoreList.empty()) return unchangedTy;
	///check consecutive pattern ... load .. store .. load ... store ...
	bool isLoad = true;
	for(std::list<Value*>::iterator II = loadStoreList.begin(), IE = loadStoreList.end(); II != IE; II++)
	{
		Value* inst = *II;
		if(isLoad)
		{
			if(isa<LoadInst>(*inst)){ isLoad = false; }else{ return changedTy; }
		}	
		else
		{
			if(isa<LoadInst>(*inst)){ return changedTy; }else{ isLoad = true; }
		}
	}
	if(!isLoad) return changedTy;
	//check type of each run .. load (run) store ...
		
	return changedTy;
}
void GlobalOptimizationsOnArrayChecks::findGenSets()
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
		this->dumpSetOfPtr(BBVB_IN);
	}
}



template <typename T> void GlobalOptimizationsOnArrayChecks::dumpSetOfPtr(std::set<T*> *set)
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

