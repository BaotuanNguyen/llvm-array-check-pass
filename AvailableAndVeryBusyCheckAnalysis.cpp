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
#include "AvailableAndVeryBusyCheckAnalysis.h"

using namespace llvm;

char AvailableAndVeryBusyCheckAnalysis::ID = 0;
static RegisterPass<AvailableAndVeryBusyCheckAnalysis> E("a-vb-analysis", "Available and Very Busy Checks Analysis", false, false);

RangeCheckSet* SetUnion(RangeCheckSet* S1, RangeCheckSet* S2)
{
	RangeCheckSet* unionSet = S1->set_union(S2);
	return unionSet;
}

RangeCheckSet* SetIntersection(RangeCheckSet* S1, RangeCheckSet* S2)
{
	RangeCheckSet* intersectSet = S1->set_intersect(S2);
	return intersectSet;
}

RangeCheckSet* SetsMeet(ListRCS* sets, RangeCheckSet*(*meet)(RangeCheckSet*, RangeCheckSet*))
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

bool AvailableAndVeryBusyCheckAnalysis::runOnFunction(Function& F)
{
	this->currentFunction = &F;
	this->createUniverse();
	this->AA = &this->getAnalysis<AliasAnalysis>();
	this->SE = &this->getAnalysis<ScalarEvolution>();
	this->dataFlowAnalysis(false);
	this->dataFlowAnalysis(true);
	return true;
}



void AvailableAndVeryBusyCheckAnalysis::createUniverse()
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
					///TODO:universe->insert(new RangeCheckExpression(ci, this->module));
					universe->set_union(new RangeCheckExpression(ci, this->module));
				}
			}
		}
	}
}

///we should probably keep a map of the affect type for a given variable, to make this faster if we already
///check a variable previously
AvailableAndVeryBusyCheckAnalysis::EffectTy AvailableAndVeryBusyCheckAnalysis::effect(BasicBlock* B, Value* v)
{
	return changedTy;
}

void AvailableAndVeryBusyCheckAnalysis::dataFlowAnalysis(bool isForward)
{
	/*
	 * available expression is a forward flow problem.
	 *
	 * iterate over the basic blocks of the function.
	 * for each basic block, i want the C_IN and C_OUT
	 *
	 */
	if(isForward){
		
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
				BB_A_OUT->erase(BB);
				// TODO
				// compare C_IN with our previous C_IN
				BB_A_OUT->insert(PairBBAndRCS(BB, C_OUT));
			}	
		}
	}
        /*
         * iterate over all instructions in a basic block BACKWARDS, like the capable students at the skating rinks
         * using the I_VB_IN data structure, i can map an instruction to its IN set.
         * the IN set i get like so:
         *  . first i call victor's function to get the gen set for it
         *  . then i call the backwards function
         * the OUT set is a union over all successors' INs
         */
        else{
		this->BB_VB_IN = new MapBBToRCS();
		///initialize all blocks sets
		for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
		{
			///in of the block is universe
			BasicBlock* BB = &*BBI;
			// TODO			
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
				BB_VB_IN->erase(BB);
				// TODO
				// compare C_IN with our previous C_IN
				BB_VB_IN->insert(PairBBAndRCS(BB, C_IN));
			}	
		}
	}
}
RangeCheckSet *AvailableAndVeryBusyCheckAnalysis::getAvailOut(BasicBlock *BB, RangeCheckSet *cInOfBlock)
{
        /*
         * available OUT sets generated, going fowards throught the instructions
         * iterate over the basic block.  
         */
	RangeCheckSet* currentRCS = cInOfBlock;
	llvm::BasicBlock::InstListType& instList = BB->getInstList();
	for(BasicBlock::InstListType::iterator II = instList.begin(), EI = instList.end(); II != EI; II++){
                if(CallInst* callInst = dyn_cast<CallInst>(&*II)){
                        const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
                        if(!callFunctionName.equals("checkLTLimit") && !callFunctionName.equals("checkGTZero")){
                                continue;
                        }
			RangeCheckExpression* rce = new RangeCheckExpression(callInst, this->module); // FIXME ? local variable doesn't get destroyed after function return, does it?
			///FIXME: deprecated method currentRCS->insert(rce);
                }
                else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II)){
			currentRCS->kill_forward(storeInst);
                }
        }
        return currentRCS;
}



RangeCheckSet *AvailableAndVeryBusyCheckAnalysis::getVBIn(BasicBlock *BB, RangeCheckSet *cOutOfBlock)
{
	/*
         * Return the range check set entering a basic block (given that basic block and its out RCS)
         *
         * Alg:
	 *      iterate over every instruction and just examine CALL and STORE instructions
	 */
        RangeCheckSet* currentRCS = cOutOfBlock;
	llvm::BasicBlock::InstListType& instList = BB->getInstList();
	for(BasicBlock::InstListType::reverse_iterator II = instList.rbegin(), EI = instList.rend(); II != EI; II++){
		if(CallInst* callInst = dyn_cast<CallInst>(&*II)){
			const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
			if(!callFunctionName.equals("checkLTLimit") && !callFunctionName.equals("checkGTZero")){
				continue;
			}
			RangeCheckExpression* rce = new RangeCheckExpression(callInst, this->module); // FIXME ? local variable doesn't get destroyed after function return, does it?
			///FIXME: deprecated method currentRCS->insert(rce);
		}
		else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II)){
			currentRCS->kill_backward(storeInst);
		}
	}
	return currentRCS;
}

void AvailableAndVeryBusyCheckAnalysis::findGenSets()
{
	/*this->VeryBusy_Gen = new MapBBToValuesSet();
	  this->Available_Gen = new MapBBToValuesSet();

	///very busy IN sets generated, going backwards throught the instructions
	for(llvm::Function::iterator IBB = this->currentFunction->begin(), EBB = this->currentFunction->end(); IBB != EBB; EBB--)
	{
	BasicBlock& BB = *IBB;
	///the last
	RangeCheckSet* currentRCS = new RangeCheckSet();
	llvm::BasicBlock::InstListType& instList = BB.getInstList();
	for(BasicBlock::InstListType::reverse_iterator II = instList.rbegin(), EI = instList.rend(); II != EI; II++){
	if(CallInst* callInst = dyn_cast<CallInst>(&*II)){
	const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
	if(!callFunctionName.equals("checkLTLimit") && !callFunctionName.equals("checkGTZero")){
	continue;
	}
	}
	else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II)){

	}
	}
	}
	///available OUT sets generated, going fowards throught the instructions
	for(llvm::Function::iterator IBB = this->currentFunction->begin(), EBB = this->currentFunction->end(); IBB != EBB; IBB++){
	BasicBlock& BB = *IBB;
	///go throught each block
	for(llvm::BasicBlock::iterator II = BB.begin(), EI = BB.end(); II != EI; II++){
	if(CallInst* callInst = dyn_cast<CallInst>(&*II)){
	const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
	if(!callFunctionName.equals("checkLTLimit") && !callFunctionName.equals("checkGTZero")){
	continue;
	}
	}
	else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II)){

	}

	}
	}*/
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
