#include "RemoveRedundantCheckPass.h"
#include "AvailableAnalysisPass.h"
#include "RangeCheckExpression.h"
#include "RangeCheckSet.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/Analysis/Dominators.h"
#include "stdlib.h"
#include <vector>

using namespace llvm;

char RemoveRedundantCheckPass::ID = 0;
static RegisterPass<RemoveRedundantCheckPass> C("remove-global", "Global redundant array bounds check removal", false, false);
	
bool RemoveRedundantCheckPass::runOnModule(Module& M)
{
	this->M = &M;
	this->availableAnalysis = &getAnalysis<AvailableAnalysisPass>();
	this->availableMap = this->availableAnalysis->I_A_OUT;
	this->removedNum = 0;

	errs() << "\n#########################################\n";
	errs() << "REMOVE REDUNDANT CHECK PASS\n";
	errs() << "#########################################\n";

	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i)
	{
		Function* func = &(*i);
		runOnFunction(&(*func));
	}
	
	errs() << "---------------------------------------------\n";
	errs() << "REMOVED REDUNDANT CHECKS #: " << this->removedNum << "\n";
	errs() << "---------------------------------------------\n";

	return false;
}

bool RemoveRedundantCheckPass::runOnFunction(Function* F)
{
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
	{
		runOnBasicBlock(i);
	}
	return true;
}

bool RemoveRedundantCheckPass::runOnBasicBlock(BasicBlock* BB)
{
	std::vector<Instruction*> redundantList;
	for (BasicBlock::iterator i = BB->begin(), e = BB->end(); i != e; ++i) 
	{
		Instruction *inst = &*i;
		 		
		if (CallInst *CI = dyn_cast<CallInst>(inst)) 
		{
			if (CI->getCalledFunction() == NULL)
				continue;

			StringRef funcName = CI->getCalledFunction()->getName();
			
			if (funcName.equals("checkLessThan"))
			{
				RangeCheckSet* current = (*availableMap)[inst];
				//errs() << "Call: " << *CI << "\n";
				//errs() << "Available Expressions: "; current->println();

				RangeCheckExpression* expr1 = new RangeCheckExpression(CI, M);
				
				for (std::vector<RangeCheckExpression>::iterator it = current->checkSet->begin(); it != current->checkSet->end(); ++it)
				{
					RangeCheckExpression* expr2 = &(*it);

					if (*expr1 == *expr2)
					{
						//errs() << "*** REMOVED ***\n";
						redundantList.push_back(CI);
					}
					else if (expr2->subsumes(expr1))
					{
						//errs() << "*** REMOVED ***\n";
						redundantList.push_back(CI);
					}
				}				
			}
		}
	}

	std::vector<Instruction*>::iterator it;

	for (it = redundantList.begin(); it != redundantList.end(); it++)
	{
		removedNum++;
		(*it)->eraseFromParent();
	}

    return true;
}
