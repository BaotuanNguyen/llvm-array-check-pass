#include "testPass.h"
#include "RangeCheckExpression.h"
#include "RangeCheckSet.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/Analysis/Dominators.h"
#include "stdlib.h"
#include <vector>

using namespace llvm;

char testPass::ID = 0;
static RegisterPass<testPass> C("test-pass", "TEST FOR FORWARD AND BACKWARD", false, false);

bool testPass::runOnModule(Module& M)
{
	this->M = &M;

	errs() << "\n#########################################\n";
	errs() << "TEST PASS\n";
	errs() << "#########################################\n";

	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i)
	{
		Function* func = &(*i);
		runOnFunction(&(*func));
	}
	
	errs() << "\n#########################################\n";
	errs() << "DONE\n";
	errs() << "#########################################\n";

	return false;
}

bool testPass::runOnFunction(Function* F)
{
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
	{
		runOnBasicBlock(i);
	}
	return true;
}

bool testPass::runOnBasicBlock(BasicBlock* BB)
{
	errs() << "Entering basic block\n";

	RangeCheckSet* current_set = new RangeCheckSet();

	for (BasicBlock::iterator i = BB->begin(), e = BB->end(); i != e; ++i) 
	{
		Instruction *inst = &*i;
		 		
		if (CallInst *CI = dyn_cast<CallInst>(inst)) 
		{
			StringRef funcName = CI->getCalledFunction()->getName();
			
			if (funcName.equals("checkLTLimit") || funcName.equals("checkGTZero"))
			{
				RangeCheckExpression* rangeExpr = new RangeCheckExpression(CI, this->M);
				errs() << "RangeExpr generated: ";
				rangeExpr->println();
				current_set = current_set->set_union(rangeExpr);
			}

			errs() << "before intersect size: " << current_set->size() << "\n";
			RangeCheckSet* intersectedSet = current_set->set_intersect(current_set);
			errs() << "after intersect size: " << intersectedSet->size() << "\n";
		}

		if (StoreInst *SI = dyn_cast<StoreInst>(inst))
		{
			errs() << "store instruction: " << *SI << "\n";
			errs() << "variable instruction: " << *(SI->getOperand(0)) << "\n";
			
			Value* op2 = (SI->getOperand(1));
			
			if (dyn_cast<AllocaInst>(op2))
			{
				current_set->kill_forward(SI);
			}		
		}
	}

	errs() << "Exiting basic block\n\n";
    return true;
}
