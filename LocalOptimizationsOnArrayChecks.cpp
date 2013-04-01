#include "LocalOptimizationsOnArrayChecks.h"
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

char LocalOptimizationsOnArrayChecks::ID = 0;
static RegisterPass<LocalOptimizationsOnArrayChecks> C("local-opts", "Local optimizations on array checks performed", false, false);


bool LocalOptimizationsOnArrayChecks::doInitialization(Module& M)
{
        // stub function. do not delete. keeps the compiler warnings and errors at bay
        return false;
}

bool LocalOptimizationsOnArrayChecks::doInitialization(Function& F)
{
        errs() << "\n#########################################\n";
        errs() << "Beginning LocalOptimizationsOnArrayChecks\n";
        errs() << "#########################################\n";
        errs() << "#########################################\n\n";
        return false;
}




bool LocalOptimizationsOnArrayChecks::runOnBasicBlock(BasicBlock& BB)
{
        errs() << "Inspecting a new basic block...\n";
        // hold an array of all of the call instructions in that basic block
        // every time we see a call instruction, loop over all the ones seen so far
        // if the exact check has already been made,
        // remove it

        /*
         * identical checks optimization:
         *  if C comes before C', and they are identical checks,
         *  then as long as the variables used in the check are not redefined between them,
         *  then we can eliminate C'
         */

        // iterate over all instructions in a basic block
        for (BasicBlock::iterator i = BB.begin(), e = BB.end(); i != e; ++i) {
                Instruction *inst = &*i;
                errs() << "Instruction: " <<  *i << "\n";

                // if it is a CallInst, then we may have a redundant check
                if (CallInst *ci = dyn_cast<CallInst> (inst)) {
                        errs() << "is call inst: " << *ci << "\n";

                        // get the called fn
                        Function *calledFn = ci->getCalledFunction();
                        errs() << "called fn: " <<  *calledFn << "\n";

                        // iterate over the operands
                        int numArgs = ci->getNumArgOperands();
                        for (int a = 0; a < numArgs; ++a) {
                                Value *v = ci->getArgOperand(a);
                                errs() << "arg " << a << ": " << *v << "\n";
                        }
                        Value *gepOpAsValue = ci->getArgOperand(0);
                        if (GEPOperator *gepOp = dyn_cast<GEPOperator> (gepOpAsValue)) {
                                errs() << "gepOp discovered\n";
                                Value *ptrOperand = gepOp->getPointerOperand();
                                errs() << "ptrOperand: " << *ptrOperand << "\n";
                                errs() << "ptrOperand name: " << ptrOperand->getName() << "\n";
                                errs() << "ptrOperand valueid: " << ptrOperand->getValueID() << "\n";
                                Value *originPointer = findOriginOfPointer(ptrOperand);
                                errs() << "originPointer name: " << originPointer->getName() << "\n";
                        }
                }
        }

        errs() << "Exiting basic block\n\n";
        return false;
}






// TODO copy-pasted from ArrayBoundsCheckPass
Value* LocalOptimizationsOnArrayChecks::findOriginOfPointer(Value* pointer)
{
	std::set<Value*> originSet;
	std::set<Value*> valuesExploredSet;
	std::queue<Value*> valuesToExplore;
	
	valuesToExplore.push(pointer);

	while(!valuesToExplore.empty())
	{
		Value* currentValue = valuesToExplore.front();
		valuesToExplore.pop();

		//if this value has been explored already, then don't explore it
		if(valuesExploredSet.count(currentValue) > 0)
		{
			continue;
		}
	
		valuesExploredSet.insert(currentValue);

		if (CastInst* CAST = dyn_cast<CastInst>(currentValue))
		{
			//get the operand being cast, this will get us to memory location
			valuesToExplore.push(CAST->getOperand(0));
		}
		else if(PHINode* PHI = dyn_cast<PHINode>(currentValue))
		{
			unsigned int i;
			for(i = 0; i < PHI->getNumIncomingValues(); i++)
			{
				valuesToExplore.push(PHI->getIncomingValue(i));
			}
		}
		else if(GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(currentValue))
		{
			valuesToExplore.push(GEP->getPointerOperand());
		}
		else
		{
			//since we have a value and we don't know what to do with it, we should
			////add it origin set, if it is the only value in the origin set then this will
			////be the origin value.
			originSet.insert(currentValue);
		}
	}

	if(originSet.size() == 1)
	{
		return *(originSet.begin());
	}
	else
	{
		return NULL;
	}
}

