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
#include "LocalAvailableAnalysisPass.h"

using namespace llvm;

char LocalAvailableAnalysisPass::ID = 0;
static RegisterPass<LocalAvailableAnalysisPass> F("local-available-analysis", "Local Available Bound Checks Analysis", false, false);

/*
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
}*/

bool LocalAvailableAnalysisPass::runOnModule(Module& M)
{
    this->module = &M;        
	this->BB_A_OUT = new MapBBToRCS();
	this->I_A_OUT = new MapInstToRCS();
	
	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i)
	{
		Function* func = &(*i);
		runOnFunction(&(*func));
	}	

	return false;
}

bool LocalAvailableAnalysisPass::runOnFunction(Function* F)
{
	this->currentFunction = F;
	this->dataFlowAnalysis();
	return true;
}

void LocalAvailableAnalysisPass::createUniverse()
{		
	this->universe = new RangeCheckSet();		
	
	for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
	{
		BasicBlock* BB = &*BBI;
		for(BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; II++)
		{
			Instruction* inst = &*II;	
			if(CallInst *ci = dyn_cast<CallInst> (inst)){
				const StringRef& callFunctionName = ci->getCalledFunction()->getName();
                        	if(callFunctionName.equals("checkLTLimit") || callFunctionName.equals("checkGTZero")){
					universe->set_union(new RangeCheckExpression(ci, this->module));
				}
			}
		}
	}
}

void LocalAvailableAnalysisPass::dataFlowAnalysis()
{
	// go through each of Basic blocks
	for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
	{
		BasicBlock* BB = &*BBI;
		this->getAvailOut(BB, new RangeCheckSet());
	}	
}
RangeCheckSet *LocalAvailableAnalysisPass::getAvailOut(BasicBlock *BB, RangeCheckSet *cInOfBlock)
{
        RangeCheckSet* currentRCS = cInOfBlock;
	llvm::BasicBlock::InstListType& instList = BB->getInstList();
	for(BasicBlock::InstListType::iterator II = instList.begin(), EI = instList.end(); II != EI; II++){
                if(CallInst* callInst = dyn_cast<CallInst>(&*II)){
                        const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
                        if(!callFunctionName.equals("checkLTLimit") && !callFunctionName.equals("checkGTZero")){
                                continue;
                        }

						//INSERT AVAILABLE INFORMATION FOR THIS CALL INSTRUCTION!
						I_A_OUT->erase(callInst);
						this->I_A_OUT->insert(PairIAndRCS(callInst, currentRCS));
					
			RangeCheckExpression* rce = new RangeCheckExpression(callInst, this->module); // FIXME ? local variable doesn't get destroyed after function return, does it?
			currentRCS->set_union(rce);
                }
				else if (false)
				{
					// NEED TO KILL VARIABLE OF WHICH ITS ADDRESS GETS LOADED
				}
                else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II)){
			currentRCS->kill_forward(storeInst);
                }
        }
        return currentRCS;
}

template <typename T> void LocalAvailableAnalysisPass::dumpSetOfPtr(std::set<T*> *set)
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
