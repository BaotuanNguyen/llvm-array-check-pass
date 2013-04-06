#include "LocalRemoveRedundantCheckPass.h"
#include "RangeCheckExpression.h"
#include "RangeCheckSet.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/Analysis/Dominators.h"
#include "stdlib.h"
#include <vector>

using namespace llvm;

char LocalRemoveRedundantCheckPass::ID = 0;
static RegisterPass<LocalRemoveRedundantCheckPass> C("local-remove-redundant-check", "Local Redundant Array Bound Check Removal", false, false);

bool LocalRemoveRedundantCheckPass::runOnModule(Module& M)
{
	this->M = &M;

	errs() << "\n#########################################\n";
	errs() << "REMOVE REDUNDANT CHECK PASS\n";
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

bool LocalRemoveRedundantCheckPass::runOnFunction(Function* F)
{
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
	{
		runOnBasicBlock(i);
	}
	return true;
}

bool LocalRemoveRedundantCheckPass::runOnBasicBlock(BasicBlock* BB)
{
	for (BasicBlock::iterator i = BB->begin(), e = BB->end(); i != e; ++i) 
	{
		Instruction *inst = &*i;
		 		
		if (CallInst *CI = dyn_cast<CallInst>(inst)) 
		{
			StringRef funcName = CI->getCalledFunction()->getName();
			
			if (funcName.equals("checkLTLimit") || funcName.equals("checkGTZero"))
			{				
			}
		}
	}
    return true;
}
