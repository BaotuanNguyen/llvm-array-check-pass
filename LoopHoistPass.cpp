#include "LoopHoistPass.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"
#include "llvm/Analysis/LoopInfo.h"
#include "stdlib.h"
#include <set>
#include <queue>

using namespace llvm;

char LoopHoistPass::ID = 0;
static RegisterPass<LoopHoistPass> Y("loop-hoist", "Propagate of checks out of loops", false, false);

bool LoopHoistPass::runOnModule(Module& M)
{
	this->M = &M;
	this->numHoisted = 0;

	errs() << "\n#########################################################################\n";
	errs() << "   Loop Hoist Pass\n";
	errs() << "###########################################################################\n";

	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i)
	{
		Function* func = &(*i);
		runOnFunction(func);
	}
	
	errs() << "-----------------------------------------------------------------------------------------\n";
	errs() << "	          Number of checks hoisted: " << this->numHoisted << "\n";
	errs() << "-----------------------------------------------------------------------------------------\n";

	return true;
}

bool LoopHoistPass::runOnFunction(Function* F)
{
	if (!(F->isDeclaration()))
	{
		this->loopInfo = &getAnalysis<LoopInfo>(*F);

		for (LoopInfo::iterator i = loopInfo->begin(), e = loopInfo->end(); i != e; i++)
		{
			Loop* loop = *i;
			runOnLoop(loop);
		}
	}
	return true;
}

bool LoopHoistPass::isInvariant(Value* operand, Loop* L)
{
	Instruction* inst = dyn_cast<Instruction>(operand);
	
	if (!inst)
	{
		return true;
	}

    MDNode* metadata = inst->getMetadata("EFFECT");

	if (metadata == NULL)
	{
		return false;
	}
	
	if (dyn_cast<MDString>(metadata->getOperand(0))->getString().equals("CHANGED"))
	{
		return false; // can't reason about "changed" variable
	}

	if (!dyn_cast<LoadInst>(metadata->getOperand(1)))
	{
		return false; // it is possible to reason about this case, but for now lets just ignore it...
	}

	Value* origin = metadata->getOperand(1);

	Instruction* originInst = dyn_cast<Instruction>(origin);

	if (!originInst)
	{
		return true;
	}
								  
	BasicBlock* originBlock = originInst->getParent();
	std::vector<BasicBlock*>* blocks = &L->getBlocksVector();
    
	for(std::vector<BasicBlock*>::iterator it = blocks->begin(), ie = blocks->end(); it != ie; ++it)
	{
		if (originBlock == *it)
		{
			return false;
		}
	}
	
	return true;
}

bool LoopHoistPass::runOnLoop(Loop *L)
{
		candidates = new std::vector<CallInst*>();

		std::vector<BasicBlock*>* blocks = &L->getBlocksVector();
		
        for(std::vector<BasicBlock*>::iterator it = blocks->begin(), ie = blocks->end(); it != ie; ++it)
		{
                BasicBlock *block = *it;
                errs() << "\nEntering a Loop Basic Block: " << block->getName() << "\n";
                
				for(BasicBlock::iterator bbIt = block->begin(), bbIe = block->end(); bbIt != bbIe; ++bbIt)
				{
                        Value *v = &*bbIt;

                        if(CallInst *ci = dyn_cast<CallInst> (v))
						{
							if (ci->getCalledFunction() == NULL)
								continue;
							
							const StringRef& callFunctionName = ci->getCalledFunction()->getName();
							
							if(callFunctionName.equals("checkLessThan"))
							{
								errs() << "Checking invariance of: " << *ci << "\n";
                                  MDNode* metadata = ci->getMetadata("VarName");
							  
								  Value* operand1 = metadata->getOperand(0);
								  Value* operand2 = metadata->getOperand(1);

								  if (isInvariant(operand1, L) && isInvariant(operand2, L))
								  {
  //                                    errs() << "Callinst: " << *ci << " is loop invariant!\n";
									  candidates->push_back(ci);
								  }
							}
						}
				}
		}

		if (candidates->size() > 0)
		{
			BasicBlock* preheader = L->getLoopPreheader();
				
			if (preheader != NULL)
			{
				errs() << "\nPreheader exists: " << preheader->getName() << "\n";
			}

			hoistTo(preheader);
		}

		delete candidates;
		return true;
}

void LoopHoistPass::hoistTo(BasicBlock *preheader)
{
	for (std::vector<CallInst*>::iterator it = candidates->begin(); it != candidates->end(); it++)
	{
		CallInst* hoistedInst = *it;
		
		errs() << "hoisting: " << *hoistedInst << "\n";
		
		hoistedInst->removeFromParent();

		Instruction* back = &(preheader->back());

	   	preheader->getInstList().insert(back, hoistedInst);

		MDNode* varNames = hoistedInst->getMetadata("VarName");
		Value* operand1 = varNames->getOperand(0);
		Value* operand2 = varNames->getOperand(1);

		if (operand1->getType() != Type::getInt64Ty(this->M->getContext()))
		{
			if (operand1->getType()->isPointerTy())
				operand1 = CastInst::CreatePointerCast(operand1, Type::getInt64Ty(this->M->getContext()), "HOIST_CHECK", hoistedInst);
			else
				operand1 = CastInst::CreateIntegerCast(operand1, Type::getInt64Ty(this->M->getContext()), true, "HOIST_CHECK", hoistedInst);
		}
		
		if (operand2->getType() != Type::getInt64Ty(this->M->getContext()))
		{
			if (operand2->getType()->isPointerTy())
				operand2 = CastInst::CreatePointerCast(operand2, Type::getInt64Ty(this->M->getContext()), "HOIST_CHECK", hoistedInst);
			else
				operand2 = CastInst::CreateIntegerCast(operand2, Type::getInt64Ty(this->M->getContext()), true, "HOIST_CHECK", hoistedInst);
		}
		
		hoistedInst->setArgOperand(0, operand1);
		hoistedInst->setArgOperand(1, operand2);
	
		numHoisted++;
	}
	return;	
}
