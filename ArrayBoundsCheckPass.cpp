
#include "llvm/User.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/InstrTypes.h"
#include "ArrayBoundsCheckPass.h"
#include <set>
#include <queue>

using namespace llvm;

char ArrayBoundsCheckPass::ID = 0;
static RegisterPass<ArrayBoundsCheckPass> X("hello", "hello pass", false, false);

bool ArrayBoundsCheckPass::runOnFunction(Function& F)
{
	this->linearizeAllInstructions(F);	
	this->numBlockVisited++;
	return true;

}

bool ArrayBoundsCheckPass::linearizeAllInstructions(Function& F)
{
	//iterator over instructions
	errs() << this <<" block " << this->numBlockVisited << " \n";	
	for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
	{
		if(GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(&*I))
		{
			errs() << *GEP << "[";
			if(Value* origin = this->findOriginOfPointer(GEP->getPointerOperand()))
			{
				errs() << *origin;
			}
			errs() << "]\n";
		}
	}
	return false;
}
/*
 * whether the instruction was linearized
 */
bool ArrayBoundsCheckPass::linearizeInstruction(Instruction* instruction)
{
	unsigned int index;
	errs() << "\n-> ("<< instruction->getNumUses() <<", " << instruction->hasName() << ") " << *instruction;
	errs() << "[ ";
	for(index = 0; index < instruction->getNumOperands(); index++)
	{
		Value* operand = instruction->getOperand(index);
		Type* opType = instruction->getType();
		errs() << *operand <<" (" << *opType << ") " <<"*** ";
	}
	errs() << " ]\n\n";
	return true;;
}
/*
 * Given a pointer in a GEP instruction, this will try to determine which
 * value the pointer came from
 */
Value* ArrayBoundsCheckPass::findOriginOfPointer(Value* pointer)
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

		if(CastInst* CAST = dyn_cast<CastInst>(currentValue))
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
			//add it origin set, if it is the only value in the origin set then this will
			//be the origin value.
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
