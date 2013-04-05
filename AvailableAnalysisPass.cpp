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
#include "AvailableAnalysisPass.h"

using namespace llvm;

char AvailableAnalysisPass::ID = 0;
static RegisterPass<AvailableAnalysisPass> F("available-analysis", "Available checks analysis is performed globally", false, false);

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

bool AvailableAnalysisPass::runOnFunction(Function& F)
{
	this->currentFunction = &F;
	this->createUniverse();
	this->dataFlowAnalysis();
	return true;
}



void AvailableAnalysisPass::createUniverse()
{		
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

void AvailableAnalysisPass::dataFlowAnalysis()
{
	this->BB_A_OUT = new MapBBToRCS();
	///initialize all blocks sets
	for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
	{
		///in of the block is universe
		BasicBlock* BB = &*BBI;
		this->BB_A_OUT->insert(PairBBAndRCS(BB, universe));
	}

	bool isChanged = true;
	while(isChanged){
		isChanged = false;
		///go throught all of the blocks
		for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++){
			BasicBlock* BB = &*BBI;
			ListRCS predRCS;
			///get a list of range check sets
			for(succ_iterator SBBI = succ_begin(BB), SBBE = succ_end(BB); SBBI != SBBE; SBBI++){
				predRCS.push_back((*BB_A_OUT)[*SBBI]);
			}
			///calculate the OUT of the block, by intersecting all successors IN's
			RangeCheckSet *C_IN = SetsMeet(&predRCS, SetIntersection);
			///calculate the IN of the block, running the functions we already created
			RangeCheckSet *C_OUT = this->getAvailOut(BB, C_IN);
			RangeCheckSet *C_OUT_P = BB_A_OUT->find(BB)->second;
			BB_A_OUT->erase(BB);
			//compare C_OUT and C_OUT_PREV
			if(!C_OUT_P->equal(C_OUT))
				isChanged = true;	
			BB_A_OUT->insert(PairBBAndRCS(BB, C_OUT));
		}	
	}
}
RangeCheckSet *AvailableAnalysisPass::getAvailOut(BasicBlock *BB, RangeCheckSet *cInOfBlock)
{
        RangeCheckSet* currentRCS = cInOfBlock;
	llvm::BasicBlock::InstListType& instList = BB->getInstList();
	for(BasicBlock::InstListType::iterator II = instList.begin(), EI = instList.end(); II != EI; II++){
                if(CallInst* callInst = dyn_cast<CallInst>(&*II)){
                        const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
                        if(!callFunctionName.equals("checkLTLimit") && !callFunctionName.equals("checkGTZero")){
                                continue;
                        }
			RangeCheckExpression* rce = new RangeCheckExpression(callInst, this->module); // FIXME ? local variable doesn't get destroyed after function return, does it?
			currentRCS->set_union(rce);
                }
                else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II)){
			currentRCS->kill_forward(storeInst);
                }
        }
        return currentRCS;
}

template <typename T> void AvailableAnalysisPass::dumpSetOfPtr(std::set<T*> *set)
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

bool AvailableAnalysisPass::doFinalization(Module& M)
{
	return false;	
}
