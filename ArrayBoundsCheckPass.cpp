
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


bool ArrayBoundsCheckPass::runOnFunction(Function& F)
{
	this->findArrayAccess(F);	
	this->numBlockVisited++;
	return true;

}

bool ArrayBoundsCheckPass::findArrayAccess(Function& F)
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
			errs() << "call inserted\n";
			Value* ten = ConstantInt::get(Type::getInt32Ty(F.getContext()), 10);
			CallInst::Create(this->arrayAccessFunction, ten, "", GEP);
		}
	}
	return false;
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
