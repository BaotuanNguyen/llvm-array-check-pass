#include "ModifyCheckPass.h"
#include "RangeCheckExpression.h"
#include "RangeCheckSet.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/Analysis/Dominators.h"
#include "stdlib.h"
#include <vector>

using namespace llvm;

char ModifyCheckPass::ID = 0;
static RegisterPass<ModifyCheckPass> C("modify-check", "Modify Array Bound Checks using very busy checks", false, false);
	
void ModifyCheckPass::modify(CallInst* callInst, RangeCheckSet* RCS, Module* M)
{
	RangeCheckExpression* expr = new RangeCheckExpression(callInst, M);
		
	for (std::vector<RangeCheckExpression>::iterator it = RCS->checkSet->begin(); it != RCS->checkSet->end(); ++it)
	{
		RangeCheckExpression* expr2 = &(*it);

		if (expr2->subsumes(expr))
		{
			errs() << "Replacing "; expr->print();
			errs() << " with "; expr2->println();

			if (expr->relOp == GTEQ)
			{
				callInst->setArgOperand(0, expr2->op1); // replace first operand
				callInst->getMetadata("VarName")->replaceOperandWith(0, expr2->op1);
			}
			else
			{
				callInst->setArgOperand(1, expr2->op2); // replace second operand
				callInst->getMetadata("VarName")->replaceOperandWith(1, expr2->op2);
			}
			return;
		}
	}
}

bool ModifyCheckPass::runOnModule(Module& M)
{
	this->M = &M;
	this->veryBusyAnalysis = &getAnalysis<VeryBusyAnalysisPass>();
	this->veryBusyMap = this->veryBusyAnalysis->I_VB_IN;

	errs() << "\n#########################################\n";
	errs() << "MODIFYCHECK PASS\n";
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

bool ModifyCheckPass::runOnFunction(Function* F)
{
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
	{
		runOnBasicBlock(i);
	}
	return true;
}

bool ModifyCheckPass::runOnBasicBlock(BasicBlock* BB)
{

	for (BasicBlock::iterator i = BB->begin(), e = BB->end(); i != e; ++i) 
	{
		Instruction *inst = &*i;
		 		
		if (CallInst *CI = dyn_cast<CallInst>(inst)) 
		{
			StringRef funcName = CI->getCalledFunction()->getName();
			
			if (funcName.equals("checkLTLimit") || funcName.equals("checkGTLimit"))
			{
				RangeCheckSet* current = (*veryBusyMap)[inst];
				errs() << "Very Busy Expressions: "; current->println();
				modify(CI, current, this->M);
			}
		}
	}
    return true;
}
