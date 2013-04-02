#include "GlobalOptimizationsOnArrayChecks.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "stdlib.h"
#include <set>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
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
	this->findVeryBusyChecks();
        return true;
}

void GlobalOptimizationsOnArrayChecks::findVeryBusyChecks()
{

	MapBBToValuesSet* VeryBusy_Gen = new MapBBToValuesSet();
	//std::map<BasicBlock*, std::set<Value*> >* VeryBusy_OUT;
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
				BBVB_IN->insert(callInst);
			}
		}

		VeryBusy_Gen->insert(PairBBToValuesSet(&BB, BBVB_IN));
		this->dumpSetOfPtr(BBVB_IN);
		//add block to map
		//VeryBusy_IN->insert
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

void GlobalOptimizationsOnArrayChecks::findAvailableChecks()
{
}

