#include "ArrayBoundsCheckPass.h"
#include "RunTimeBoundsChecking.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"
#include "RangeCheckExpression.h"

using namespace llvm;

bool RangeCheckExpression::subsumes(RangeCheckExpression* expr)
{
	errs() << "subsume?\n";
	// a rangecheckexpression subsumes itself
	if ((*this) == (*expr))
	{
		return true;
	}

	// if relops are different, cannot subsume
	if (this->relOp != expr->relOp)
	{
		return false;
	}
	else
	{
		// GTEQ check expressions cannot subsume one another unless they are equal
		if ((this->relOp == GTEQ) && (expr->relOp == GTEQ))
		{
			return false;
		}
		else // both are LT expressions
		{
			if (this->op1 != expr->op1)
			{
				return false;
			}	
			else
			{
				if (ConstantInt* CI1 = dyn_cast<ConstantInt>(op1))
				{
					uint64_t value1 = CI1->getSExtValue();
					
					if (ConstantInt* CI2 = dyn_cast<ConstantInt>(op2))
					{
						uint64_t value2 = CI2->getSExtValue();
						return (value1 < value2);
					}
					else if (ConstantFP* CF2 = dyn_cast<ConstantFP>(op2))
					{
						double value2 = (CF2->getValueAPF()).convertToDouble();
						return (value1 < value2);
					}
				}
				else if (ConstantFP* CF1 = dyn_cast<ConstantFP>(op1))
				{
					double value1 = (CF1->getValueAPF()).convertToDouble();
					
					if (ConstantInt* CI2 = dyn_cast<ConstantInt>(op2))
					{
						uint64_t value2 = CI2->getSExtValue();
						return (value1 < value2);
					}
					else if (ConstantFP* CF2 = dyn_cast<ConstantFP>(op2))
					{
						double value2 = (CF2->getValueAPF()).convertToDouble();
						return (value1 < value2);
					}
				}
				else
				{
					return false;
				}
			}
		}
	}

	errs() << "ERROR (IN SUBSUME): THIS SHOULD NOT BE REACHED!!!\n";
	return false;
}
		
void RangeCheckExpression::print()
{
		std::string relOpStr;
		if (this->relOp == GTEQ)
		{
			relOpStr = " <= ";
			ConstantInt* zero = dyn_cast<ConstantInt>(op1);
			errs() << (zero->getSExtValue()) << relOpStr << (op2->getName().str());
		}
		else
		{
			relOpStr = " < ";
			
			if (ConstantInt* CI = dyn_cast<ConstantInt>(op1))
			{
				errs() << CI->getSExtValue();															
			}
			else if (ConstantFP* CF = dyn_cast<ConstantFP>(op1))
			{
				errs() << (CF->getValueAPF()).convertToDouble();
			}
			else
			{
				errs() << op1->getName().str();
			}
			
			errs() << relOpStr;

			if (ConstantInt* CI = dyn_cast<ConstantInt>(op2))
			{
				errs() << CI->getSExtValue();															
			}
			else if (ConstantFP* CF = dyn_cast<ConstantFP>(op2))
			{
				errs() << (CF->getValueAPF()).convertToDouble();
			}
			else
			{
				errs() << op2->getName().str();
			}
		}
}

void RangeCheckExpression::println()
{
		std::string relOpStr;
		if (this->relOp == GTEQ)
		{
			relOpStr = " <= ";
			ConstantInt* zero = dyn_cast<ConstantInt>(op1);
			errs() << (zero->getSExtValue()) << relOpStr << (op2->getName().str());
		}
		else
		{
			relOpStr = " < ";
			
			if (ConstantInt* CI = dyn_cast<ConstantInt>(op1))
			{
				errs() << CI->getSExtValue();															
			}
			else if (ConstantFP* CF = dyn_cast<ConstantFP>(op1))
			{
				errs() << (CF->getValueAPF()).convertToDouble();
			}
			else
			{
				errs() << op1->getName().str();
			}
			
			errs() << relOpStr;

			if (ConstantInt* CI = dyn_cast<ConstantInt>(op2))
			{
				errs() << CI->getSExtValue();															
			}
			else if (ConstantFP* CF = dyn_cast<ConstantFP>(op2))
			{
				errs() << (CF->getValueAPF()).convertToDouble();
			}
			else
			{
				errs() << op2->getName().str();
			}
		}

		errs() << "\n";
}

RangeCheckExpression::RangeCheckExpression(CallInst* inst, Module* M)
{
	StringRef funcName = inst->getCalledFunction()->getName();
	LLVMContext& context = M->getContext();

	if (!funcName.equals("checkLTLimit") && !funcName.equals("checkGTZero"))
	{
		errs() << "ERROR! Can only create RangeCheckExpression from checkLTLimit call or checkGTZero call\n";
	}
	
	MDNode* metadata = inst->getMetadata("VarName");
		
	if (metadata == NULL)
	{
		errs() << "ERROR! call inst does not have VarName metadata!\n";
	}
	
	if (funcName.equals("checkGTZero"))
	{
		this->op1 = ConstantInt::get(Type::getInt64Ty(context), 0);
		this->op2 = metadata->getOperand(0);
		this->relOp = GTEQ;
	}
	else
	{
		this->op1 = metadata->getOperand(0);
		this->op2 = metadata->getOperand(1);
		this->relOp = LT;
	}
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
		else if (dyn_cast<Constant>(this->op2) && dyn_cast<Constant>(other.op2)) // RHS consists of constants for both
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
