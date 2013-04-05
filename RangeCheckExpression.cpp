#include "ArrayBoundsCheckPass.h"
#include "RunTimeBoundsChecking.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"
#include "RangeCheckExpression.h"

bool RangeCheckExpression::subsumes(RangeCheckExpression* expr)
{
	return true;
}

bool RangeCheckExpression::operator==(const RangeCheckExpression& other) const
{
	if (this->relOp != other.relOp)
	{
		return false;
	}
	else if (this->relOp == GTEQ && other.relOp == GTEQ)
	{
		return (this->op2 == other.op2);
	}
	else // relop same
	{
		if (this->op1 != other.op1) // variables on LHS is different
		{
			return false;
		}
		else if (cast<Constant>(this->op2) && cast<Constant>(other.op2)) // RHS consists of constants for both
		{
			if (ConstantInt* CI1 = dyn_cast<ConstantInt>(this->op2))
			{
				uint64_t intVal1 = CI1->getSExtValue();
				
				if (ConstantInt* CI2 = dyn_cast<ConstantInt>(other.op2))
				{
					uint64_t intVal2 = CI2->getSExtValue();
					return (intVal1 == intVal2);	
				}
				else if (ConstantFP* CF2 = dyn_cast<ConstantFP>(other.op2))
				{
					double doubleVal2 = (CF2->getValueAPF()).convertToDouble();
					return (intVal1 == doubleVal2);
				}	
				else
				{
					errs() << "ERROR! UNKNOWN CONSTANT TYPE!!\n";
					return false;
				}
			}
			else if (ConstantFP* CF1 = dyn_cast<ConstantFP>(this->op2))
			{
				double doubleVal1 = (CF1->getValueAPF()).convertToDouble();
				
				if (ConstantInt* CI2 = dyn_cast<ConstantInt>(other.op2))
				{
					uint64_t intVal2 = CI2->getSExtValue();
					return (doubleVal1 == intVal2);	
				}
				else if (ConstantFP* CF2 = dyn_cast<ConstantFP>(other.op2))
				{
					double doubleVal2 = (CF2->getValueAPF()).convertToDouble();
					return (doubleVal1 == doubleVal2);
				}	
				else
				{
					errs() << "ERROR! UNKNOWN CONSTANT TYPE!!\n";
					return false;
				}
			}
			else
			{
				errs() << "ERROR! UNKNOWN CONSTANT TYPE!!\n";
				return false;
			}
		}
		else
		{
			return this->op2 == other.op2;
		}
	}
}
