#include "RangeCheckExpression.h"
#include "RangeCheckSet.h"
#include <set>
#include "llvm/InstrTypes.h"

using namespace llvm;
		
RangeCheckSet* RangeCheckSet::set_intersect(RangeCheckSet* s)
{
	RangeCheckSet* intersectSet = new RangeCheckSet();

	for (std::vector<RangeCheckExpression>::iterator it1 = this->checkSet->begin(); it1 != this->checkSet->end(); ++it1)
	{
		for (std::vector<RangeCheckExpression>::iterator it2 = s->checkSet->begin(); it2 != s->checkSet->end(); ++it2)
		{
			if (*it1 == *it2)
			{
				intersectSet->insert(*it1);
			}
			else if (it2->subsumes(&(*it1)))
			{
				intersectSet->insert(*it1);
			}
			else if (it1->subsumes(&(*it2)))
			{
				intersectSet->insert(*it2);
			}
		}
	}
	
	return intersectSet;
}

RangeCheckSet* RangeCheckSet::set_union(RangeCheckExpression* expr)
{
	RangeCheckSet* unionedSet = this->copy();

	std::vector<RangeCheckExpression>::iterator it = unionedSet->checkSet->begin();

	for (;it != unionedSet->checkSet->end(); ++it)
	{
		RangeCheckExpression current = *it;

		//errs() << "union: "; current.print(); errs() << " AND ";expr->println();
		
		if (current.subsumes(expr))
		{
//			errs() << "\t\t";
//			current.print();
//			errs() << " SUBSUMES ";
//			expr->println();
			//if expr is not used then delete it
			delete expr;
			return unionedSet;
		}	
		else if (expr->subsumes(&current))
		{
			int index = (it - unionedSet->checkSet->begin());
			unionedSet->checkSet->at(index) = *expr;

//			errs() << "\t\t";
//			expr->print();
//			errs() << " SUBSUMES ";
//			current.println();
			//if expr is not used then delete it
			delete expr;	
			return unionedSet;
		}
	}

	unionedSet->insert(*expr);
	return unionedSet;
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
	
		//errs() << "Value being stored: " << *v << "\n";

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
					variableBeingChecked = dyn_cast<Instruction>(currentCheck->left);
				else
					variableBeingChecked = dyn_cast<Instruction>(currentCheck->right);

				if (variableAffected != variableBeingChecked)
				{
					return true;
				}
				else
				{
					if (variablePos == 0)
					{
						if (mdstr->getString().equals("INCREMENT") || mdstr->getString().equals("UNCHANGED"))
							return false;
						else
							return true;
					}
					else
					{
						if (mdstr->getString().equals("DECREMENT") || mdstr->getString().equals("UNCHANGED"))
							return false;
						else
							return true;
					}
				}
			}
		}
	}
}

void RangeCheckSet::kill_backward(Instruction* store, Module* M)
{
	bool erased = false;
	
	Value* valueBeingStored = store->getOperand(0);
	Instruction* variableBeingStored = dyn_cast<Instruction>(store->getOperand(1));

	if (valueBeingStored->getType()->isPointerTy())
	{
		for (std::vector<RangeCheckExpression>::iterator it = this->checkSet->begin(); it != this->checkSet->end();)
		{
			erased = false;
			RangeCheckExpression* currentCheck = &(*it);
			
			if (Instruction* op1 = dyn_cast<Instruction>(currentCheck->left))
			{
				if (op1 == valueBeingStored)
				{
					//errs() << "\t\t"; (*it).print(); errs() << " got KILLED due to its address being copied into a pointer!\n";
					this->checkSet->erase(it);
					erased = true;
				}
			}
			else if (Instruction* op2 = dyn_cast<Instruction>(currentCheck->right))
			{
				if (op2 == valueBeingStored)
				{
					//errs() << "\t\t"; (*it).print(); errs() << " got KILLED due to its address being copied into a pointer!\n";
					this->checkSet->erase(it);
					erased = true;
				}
			}
			
			if (!erased)
				it++;
		}
		return; // a pointer cannot be a part of any range checks. Just quit now
	}

	erased = false;

	// ignoer function arguments since they cannot possibly kill any checks
	if (dyn_cast<Argument>(valueBeingStored))
		return;

	for (std::vector<RangeCheckExpression>::iterator it = this->checkSet->begin();it != this->checkSet->end();)
	{
		erased = false;
		RangeCheckExpression* currentCheck = &(*it);
		
		if (Instruction* op1 = dyn_cast<Instruction>(currentCheck->left))
		{
			if (op1 == variableBeingStored)
			{
				if (doValueKillCheckBackward(currentCheck, valueBeingStored, 0))
				{
//					errs() << "\t\t"; (*it).print(); errs() << " got KILLED\n";
					this->checkSet->erase(it);
					erased = true;
				}
			}
		}
		else if (Instruction* op2 = dyn_cast<Instruction>(currentCheck->right))
		{
			if (op2 == variableBeingStored)
			{
				if (doValueKillCheckBackward(currentCheck, valueBeingStored, 1))
				{
//					errs() << "\t\t"; (*it).print(); errs() << " got KILLED\n";
					this->checkSet->erase(it);
					erased = true;
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
					variableBeingChecked = dyn_cast<Instruction>(currentCheck->left);
				else
					variableBeingChecked = dyn_cast<Instruction>(currentCheck->right);

				if (variableAffected != variableBeingChecked)
				{
					return true;
				}
				else
				{
					if (variablePos == 0)
					{
						if (mdstr->getString().equals("DECREMENT") || mdstr->getString().equals("UNCHANGED"))
							return false;
						else
							return true;
					}
					else
					{
						if (mdstr->getString().equals("INCREMENT") || mdstr->getString().equals("UNCHANGED"))
							return false;
						else
							return true;
					}
				}
			}
		}
	}
}

void RangeCheckSet::kill_forward(Instruction* store, Module* M)
{
	bool erased = false;
	std::vector<RangeCheckExpression>::iterator it = this->checkSet->begin();
	Value* valueBeingStored = store->getOperand(0);
	Instruction* variableBeingStored = dyn_cast<Instruction>(store->getOperand(1));
	
	if (valueBeingStored->getType()->isPointerTy())
	{
		for (std::vector<RangeCheckExpression>::iterator it = this->checkSet->begin();it != this->checkSet->end();)
		{
			erased = false;
			RangeCheckExpression* currentCheck = &(*it);
			
			if (Instruction* op1 = dyn_cast<Instruction>(currentCheck->left))
			{
				if (op1 == valueBeingStored)
				{
//					errs() << "\t\t"; (*it).print(); errs() << " got KILLED due to its address being copied into a pointer!\n";
					this->checkSet->erase(it);
					erased = true;
				}
			}
			else if (Instruction* op2 = dyn_cast<Instruction>(currentCheck->right))
			{
				if (op2 == valueBeingStored)
				{
//					errs() << "\t\t"; (*it).print(); errs() << " got KILLED due to its address being copied into a pointer!\n";
					this->checkSet->erase(it);
					erased = true;
				}
			}
			
			if (!erased)
				it++;
		}
		return; // a pointer cannot be a part of any range checks. Just quit now
	}
	
	// ignoer function arguments since they cannot possibly kill any checks
	if (dyn_cast<Argument>(valueBeingStored))
		return;

	for (; it != this->checkSet->end(); )
	{
		erased = false;
		RangeCheckExpression* currentCheck = &(*it);
			
		if (Instruction* op1 = dyn_cast<Instruction>(currentCheck->left))
		{
			if (op1 == variableBeingStored)
			{
				if (doValueKillCheckForward(currentCheck, valueBeingStored, 0))
				{
					// KILL IT
					//errs() << "\t\t"; (*it).print(); errs() << " got KILLED\n";
					this->checkSet->erase(it);
					erased = true;
				}
			}
		}
		else if (Instruction* op2 = dyn_cast<Instruction>(currentCheck->right))
		{
			if (op2 == variableBeingStored)
			{
				if (doValueKillCheckForward(currentCheck, valueBeingStored, 1))
				{
					// KILL IT
					//errs() << "\t\t"; (*it).print(); errs() << " got KILLED\n";
					this->checkSet->erase(it);
					erased = true;
				}
			}
		}
	
		if (!erased)
			it++;
	}
	return;
}
