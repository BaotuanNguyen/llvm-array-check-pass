#include "RangeCheckExpression.h"
#include "RangeCheckSet.h"
#include <set>
#include "llvm/InstrTypes.h"

using namespace llvm;
		
RangeCheckSet* RangeCheckSet::set_union(RangeCheckSet* s)
{
	return NULL;
}

RangeCheckSet* RangeCheckSet::set_intersect(RangeCheckSet* s)
{
	return NULL;
}

bool RangeCheckSet::doValueKillCheckBackward(RangeCheckExpression* currentCheck, Value* v, int variablePos)
{
	// if we are redefining a variable with a constant, it should kill this check immediately
	if (dyn_cast<ConstantInt>(v) || dyn_cast<ConstantFP>(v))
	{
		return true;
	}
	else
	{
		Instruction* valueBeingStored = dyn_cast<Instruction>(v); // MUST hold
	
		if (!(valueBeingStored->getMetadata("EFFECT")))
		{
			return true; // no data available
		}	
		else
		{
			MDNode* effects = valueBeingStored->getMetadata("EFFECT");
			MDString* mdstr = dyn_cast<MDString>(effects->getOperand(0));
			
			if (mdstr->getString().equals("CHANGED"))
			{
				return true;
			}
			else
			{
				Instruction* variableAffected = dyn_cast<Instruction>(effects->getOperand(1));
				Instruction* variableBeingChecked;
				
				if (variablePos == 0)
					variableBeingChecked = dyn_cast<Instruction>(currentCheck->op1);
				else
					variableBeingChecked = dyn_cast<Instruction>(currentCheck->op2);

				if (variableAffected != variableBeingChecked)
				{
					return true;
				}
				else
				{
					if (variablePos == 0)
					{
						if (mdstr->getString().equals("INCREMENT") || mdstr->getString().equals("UNCHANGED"))
						{
							return false;
						}
						else
						{
							return true;
						}
						
					}
					else
					{
						if (mdstr->getString().equals("DECREMENT") || mdstr->getString().equals("UNCHANGED"))
						{
							return false;
						}
						else
						{
							return true;
						}
					}
				}
			}
		}
	}
}

void RangeCheckSet::kill_backward(Instruction* store)
{
	bool erased = false;
	std::vector<RangeCheckExpression>::iterator it = this->checkSet->begin();
	
	Value* valueBeingStored = store->getOperand(0);
	Instruction* variableBeingStored = dyn_cast<Instruction>(store->getOperand(1));

	for (;it != this->checkSet->end();)
	{
		erased = false;
		RangeCheckExpression* currentCheck = &(*it);
		
		if (currentCheck->relOp == GTEQ)
		{
			if (Instruction* op2 = dyn_cast<Instruction>(currentCheck->op2)) // this case must hold
			{
				if (op2 == variableBeingStored)
				{
					if (doValueKillCheckBackward(currentCheck, valueBeingStored, 1))
					{
						// KILL IT
						errs() << "KILLED:";
						(*it).println();
						this->checkSet->erase(it);
						erased = true;
					}
				}
			}
			else
			{
				errs() << "ERROR! second operand of GTEQ check is not a variable\n";
			}
		}
		else
		{
			if (Instruction* op1 = dyn_cast<Instruction>(currentCheck->op1))
			{
				if (op1 == variableBeingStored)
				{
					if (doValueKillCheckBackward(currentCheck, valueBeingStored, 0))
					{
						// KILL IT
						errs() << "KILLED:";
						(*it).println();
						this->checkSet->erase(it);
						erased = true;
					}
				}
			}
			else if (Instruction* op2 = dyn_cast<Instruction>(currentCheck->op2))
			{
				if (op2 == variableBeingStored)
				{
					if (doValueKillCheckBackward(currentCheck, valueBeingStored, 1))
					{
						// KILL IT
						errs() << "KILLED:";
						(*it).println();
						this->checkSet->erase(it);
						erased = true;
					}
				}
			}
		}

		if (!erased)
			it++;
	}
	return;
}

bool RangeCheckSet::doValueKillCheckForward(RangeCheckExpression* currentCheck, Value* v, int variablePos)
{
	// if we are redefining a variable with a constant, it should kill this check immediately
	if (dyn_cast<ConstantInt>(v) || dyn_cast<ConstantFP>(v))
	{
		return true;
	}
	else
	{
		Instruction* valueBeingStored = dyn_cast<Instruction>(v); // MUST hold
	
		if (!(valueBeingStored->getMetadata("EFFECT")))
		{
			return true; // no data available
		}	
		else
		{
			MDNode* effects = valueBeingStored->getMetadata("EFFECT");
			MDString* mdstr = dyn_cast<MDString>(effects->getOperand(0));
			
			if (mdstr->getString().equals("CHANGED"))
			{
				return true;
			}
			else
			{
				Instruction* variableAffected = dyn_cast<Instruction>(effects->getOperand(1));
				Instruction* variableBeingChecked;
				
				if (variablePos == 0)
					variableBeingChecked = dyn_cast<Instruction>(currentCheck->op1);
				else
					variableBeingChecked = dyn_cast<Instruction>(currentCheck->op2);

				if (variableAffected != variableBeingChecked)
				{
					return true;
				}
				else
				{
					if (variablePos == 0)
					{
						if (mdstr->getString().equals("DECREMENT") || mdstr->getString().equals("UNCHANGED"))
						{
							return false;
						}
						else
						{
							return true;
						}
						
					}
					else
					{
						if (mdstr->getString().equals("INCREMENT") || mdstr->getString().equals("UNCHANGED"))
						{
							return false;
						}
						else
						{
							return true;
						}
					}
				}
			}
		}
	}
}

void RangeCheckSet::kill_forward(Instruction* store)
{
	bool erased = false;
	std::vector<RangeCheckExpression>::iterator it = this->checkSet->begin();
	Value* valueBeingStored = store->getOperand(0);
	Instruction* variableBeingStored = dyn_cast<Instruction>(store->getOperand(1));

	for (; it != this->checkSet->end(); )
	{
		erased = false;
		RangeCheckExpression* currentCheck = &(*it);

		if (currentCheck->relOp == GTEQ)
		{
			if (Instruction* op2 = dyn_cast<Instruction>(currentCheck->op2)) // this case must hold
			{
				if (op2 == variableBeingStored)
				{
					if (doValueKillCheckForward(currentCheck, valueBeingStored, 1))
					{
						// KILL IT
						errs() << "KILLED:";
						(*it).println();
						this->checkSet->erase(it);
						erased = true;
					}
				}
			}
			else
			{
				errs() << "ERROR! second operand of GTEQ check is not a variable\n";
			}
		}
		else
		{
			if (Instruction* op1 = dyn_cast<Instruction>(currentCheck->op1))
			{
				if (op1 == variableBeingStored)
				{
					if (doValueKillCheckForward(currentCheck, valueBeingStored, 0))
					{
						// KILL IT
						errs() << "KILLED:";
						(*it).println();
						this->checkSet->erase(it);
						erased = true;
					}
				}
			}
			else if (Instruction* op2 = dyn_cast<Instruction>(currentCheck->op2))
			{
				if (op2 == variableBeingStored)
				{
					if (doValueKillCheckForward(currentCheck, valueBeingStored, 1))
					{
						// KILL IT
						errs() << "KILLED:";
						(*it).println();
						this->checkSet->erase(it);
						erased = true;
					}
				}
			}
		}

		if (!erased)
			it++;
	}
	return;
}
