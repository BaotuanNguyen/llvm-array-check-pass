#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/ADT/ilist.h"
#include "llvm/Support/CFG.h"
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
#include "VeryBusyAnalysisPass.h"

using namespace llvm;

char VeryBusyAnalysisPass::ID = 0;
static RegisterPass<VeryBusyAnalysisPass> E("very-busy-analysis", "Very Busy Checks Analysis", false, false);

/*RangeCheckSet* SetUnion(RangeCheckSet* S1, RangeCheckSet* S2)
  {
  RangeCheckSet* unionSet = S1->set_union(S2);
  return unionSet;
  }*/

static RangeCheckSet* SetIntersection(RangeCheckSet* S1, RangeCheckSet* S2)
{
	RangeCheckSet* intersectSet = S1->set_intersect(S2);
	return intersectSet;
}

static RangeCheckSet* SetsMeet(ListRCS* sets, RangeCheckSet*(*meet)(RangeCheckSet*, RangeCheckSet*))
{
	RangeCheckSet* lastSet = new RangeCheckSet();
	for(std::list<RangeCheckSet*>::iterator II = sets->begin(), IE = sets->end(); II != IE; II++){
		RangeCheckSet* currentSet = meet(lastSet, *II);
		//this may be a little slow but I am just trying to finish this
		delete lastSet;
		lastSet = currentSet;
	}
	return lastSet;
}

bool VeryBusyAnalysisPass::runOnFunction(Function& F)
{
	this->currentFunction = &F;
	this->createUniverse();
	this->dataFlowAnalysis();
	return false;
}



void VeryBusyAnalysisPass::createUniverse()
{		
	this->universe = new RangeCheckSet();
	for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
	{
		BasicBlock* BB = &*BBI;
		for(BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; II++)
		{
			Instruction* inst = &*II;	
			if(CallInst *ci = dyn_cast<CallInst> (inst))
			{
				const StringRef& callFunctionName = ci->getCalledFunction()->getName();
				if(callFunctionName.equals("checkLTLimit") || callFunctionName.equals("checkGTZero"))
				{
					universe->set_union(new RangeCheckExpression(ci, this->module));
				}
			}
		}
	}
}

void VeryBusyAnalysisPass::dataFlowAnalysis()
{
	this->BB_VB_IN = new MapBBToRCS();
	///initialize all blocks sets
	for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
	{
		///in of the block is universe
		BasicBlock* BB = &*BBI;
		this->BB_VB_IN->insert(PairBBAndRCS(BB, universe));
	}

	bool isChanged = true;
	while(isChanged){
		isChanged = false;
		///go throught all of the blocks
		for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++){
			BasicBlock* BB = &*BBI;
			ListRCS succsRCS;
			///get a list of range check sets
			for(pred_iterator PBBI = pred_begin(BB), PBBE = pred_end(BB); PBBI != PBBE; PBBI++){
				succsRCS.push_back((*BB_VB_IN)[*PBBI]);
			}
			///calculate the OUT of the block, by intersecting all successors IN's
			RangeCheckSet *C_OUT = SetsMeet(&succsRCS, SetIntersection);
			///calculate the IN of the block, running the functions we already created
			RangeCheckSet *C_IN = this->getVBIn(BB, C_OUT);
			RangeCheckSet *C_IN_P = BB_VB_IN->find(BB)->second;
			BB_VB_IN->erase(BB);
			// compare C_IN with our previous C_IN
			if(!C_IN_P->equal(C_IN))
				isChanged = true;	
			BB_VB_IN->insert(PairBBAndRCS(BB, C_IN));
		}
	}
	//errs() << "VERY BUSY: " << *BB_VB_IN[*(this->currentFunction->begin())] << "\n";	
}

RangeCheckSet *VeryBusyAnalysisPass::getVBIn(BasicBlock *BB, RangeCheckSet *cOutOfBlock)
{
	RangeCheckSet* currentRCS = cOutOfBlock;
	llvm::BasicBlock::InstListType& instList = BB->getInstList();
	for(BasicBlock::InstListType::reverse_iterator II = instList.rbegin(), EI = instList.rend(); II != EI; II++){
		if(CallInst* callInst = dyn_cast<CallInst>(&*II)){
			const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
			if(!callFunctionName.equals("checkLTLimit") && !callFunctionName.equals("checkGTZero")){
				continue;
			}
			RangeCheckExpression* rce = new RangeCheckExpression(callInst, this->module); // FIXME ? local variable doesn't get destroyed after function return, does it?
			currentRCS->set_union(rce);
		}
		else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II)){
			currentRCS->kill_backward(storeInst);
		}
	}
	return currentRCS;
}

template <typename T> void VeryBusyAnalysisPass::dumpSetOfPtr(std::set<T*> *set)
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

bool VeryBusyAnalysisPass::doFinalization(Module& M)
{
	return false;	
}
