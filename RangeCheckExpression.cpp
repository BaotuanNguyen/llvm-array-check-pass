#include "ArrayBoundsCheckPass.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"
#include "RangeCheckExpression.h"

using namespace llvm;

bool RangeCheckExpression::subsumes(RangeCheckExpression* expr)
{
	// a rangecheckexpression subsumes itself
	//if ((*this) == (*expr))
	//{
	//	return true;
	//}

	// if relops are different, cannot subsume
	if (this->relOp != expr->relOp)
	{
		return false;
	}

	if ((this->relOp == GTEQ) && (expr->relOp == GTEQ)) // both are GT (operand1 is constant)
	{
		if (this->op2 != expr->op2)
			return false;

		ConstantInt* CI1 = dyn_cast<ConstantInt>(this->op1);
		ConstantInt* CI2 = dyn_cast<ConstantInt>(expr->op1);

		int64_t val1 = CI1->getSExtValue();
		int64_t val2 = CI2->getSExtValue();

		return (val1 > val2);
	}
	else // both are LT expressions
	{
		if (this->op1 != expr->op1)
			return false;
		
		if (ConstantInt* CI1 = dyn_cast<ConstantInt>(this->op2))
		{
			int64_t value1 = CI1->getSExtValue();
			
			if (ConstantInt* CI2 = dyn_cast<ConstantInt>(expr->op2))
			{
				int64_t value2 = CI2->getSExtValue();
				return (value1 < value2);
			}
			else if (dyn_cast<Instruction>(expr->op2))
			{
				// VALUE NUMBERING GOES HERE!!!!!!!!!!!!!!!!!!!!
				// Find the possible constant value of this inst!
				// If not constant, return false
				return false;
			}
			else
			{
				errs() << "ERROR!! UNKNOWN OPERAND!!\n";
				return false;
			}
		}
		else if (dyn_cast<Instruction>(this->op2))
		{
			// VALUE NUMBERING GOES HERE!!!!!!!!!!!!!!!!!!!!
			// Find the possible constant value of this inst!
			// If not constant, return false
			return false;
		}
		else
		{
			errs() << "ERROR!! UNKNOWN OPERAND!!\n";
			return false;
		}
	}

	errs() << "ERROR (IN SUBSUME): THIS SHOULD NOT BE REACHED!!!\n";
	return false;
}
		
void RangeCheckExpression::print()
{
	std::string relOpStr;
	
	if (this->relOp == GTEQ)
		relOpStr = " <= ";
	else
		relOpStr = " < ";

	if (ConstantInt* CI = dyn_cast<ConstantInt>(op1))
	{
		errs() << CI->getSExtValue();															
	}
	else
	{
		if (!(op1->hasName()))
			errs() << "temp";
		else
			errs() << op1->getName().str();
	}
	
	errs() << relOpStr;

	if (ConstantInt* CI = dyn_cast<ConstantInt>(op2))
	{
		errs() << CI->getSExtValue();															
	}
	else
	{
		if (!(op2->hasName()))
			errs() << "temp";
		else
			errs() << op2->getName().str();
	}
}

void RangeCheckExpression::println()
{
	std::string relOpStr;


	if (this->relOp == GTEQ)
		relOpStr = " <= ";
	else
		relOpStr = " < ";
	

	if (ConstantInt* CI = dyn_cast<ConstantInt>(op1))
	{
		errs() << CI->getSExtValue();															
	}
	else
	{
		if (!(op1->hasName()))
		{
			errs() << "temp";
		}
		else
			errs() << op1->getName().str();
	}
	
	errs() << relOpStr;

	if (ConstantInt* CI = dyn_cast<ConstantInt>(op2))
	{
		errs() << CI->getSExtValue();															
	}
	else
	{
		if (!(op2->hasName()))
			errs() << "temp";
		else
			errs() << op2->getName().str();
	}

	errs() << "\n";
}

RangeCheckExpression::RangeCheckExpression(CallInst* inst, Module* M)
{
	StringRef funcName = inst->getCalledFunction()->getName();
	LLVMContext& context = M->getContext();

	if (!funcName.equals("checkLTLimit") && !funcName.equals("checkGTLimit"))
	{
		errs() << "ERROR! Can only create RangeCheckExpression from checkLTLimit call or checkGTLimit call\n";
	}
	
	MDNode* metadata = inst->getMetadata("VarName");
		
	if (metadata == NULL)
	{
		errs() << "ERROR! call inst does not have VarName metadata!\n";
	}

	if (funcName.equals("checkGTLimit"))
	{
		Value* operand2 = metadata->getOperand(1);

		if (dyn_cast<AllocaInst>(operand2) || dyn_cast<GlobalVariable>(operand2)) // operand 2 is a variable (not a temporary)
		{
			this->op1 = metadata->getOperand(0); // guaranteed to be a constant for GT checks
			this->op2 = metadata->getOperand(1);
			this->relOp = GTEQ;
		}
		else // operand is a temporary variable
		{
			Instruction* op2inst = dyn_cast<Instruction>(operand2);
			MDNode* effects = op2inst->getMetadata("EFFECT");
			MDString* mdstr = dyn_cast<MDString>(effects->getOperand(0));

			if (mdstr->getString().equals("CHANGED"))
			{
				this->op1 = metadata->getOperand(0); // guaranteed to be a constant for GT checks
				this->op2 = metadata->getOperand(1);
				this->relOp = GTEQ;
			}
			else
			{
				// transforms 0 < n+1 into -1 < n
				ConstantInt* op1Int = dyn_cast<ConstantInt>(metadata->getOperand(0));
				ConstantInt* addInt = dyn_cast<ConstantInt>(effects->getOperand(2));
				int64_t op1IntVal = op1Int->getSExtValue();
				int64_t addIntVal = addInt->getSExtValue();
				int64_t newOp1Val = op1IntVal - addIntVal;
			
				ConstantInt* newVal = ConstantInt::get(Type::getInt64Ty(context), newOp1Val);
				
				this->op1 = newVal;
				this->op2 = effects->getOperand(1);
				this->relOp = GTEQ;
			}
			
		}
	}
	else
	{
		Value* operand1 = metadata->getOperand(0);
		Value* operand2 = metadata->getOperand(1);
		
		if (dyn_cast<AllocaInst>(operand1) || dyn_cast<GlobalVariable>(operand1)) // operand 1 is a variable (not a temporary)
		{
			this->op1 = operand1;
			this->op2 = operand2;
			this->relOp = LT;
		}
		else // operand 1 is a temporary
		{
			Instruction* op1inst = dyn_cast<Instruction>(operand1);
			MDNode* effects = op1inst->getMetadata("EFFECT");
			MDString* mdstr = dyn_cast<MDString>(effects->getOperand(0));
			
			
			if (mdstr->getString().equals("CHANGED"))
			{
				this->op1 = operand1;
				this->op2 = operand2;
				this->relOp = LT;
			}
			else
			{
				if (dyn_cast<ConstantInt>(operand2)) // operand 2 is a constant
				{
					// transforms n+1 < 5 into n < 4
					ConstantInt* op2Int = dyn_cast<ConstantInt>(metadata->getOperand(1));
					ConstantInt* addInt = dyn_cast<ConstantInt>(effects->getOperand(2));
					int64_t op2IntVal = op2Int->getSExtValue();
					int64_t addIntVal = addInt->getSExtValue();
					int64_t newOp2Val = op2IntVal - addIntVal;
				
					ConstantInt* newVal = ConstantInt::get(Type::getInt64Ty(context), newOp2Val);
						
					this->op1 = effects->getOperand(1);
					this->op2 = newVal;
					this->relOp = LT;
				}
				else
				{
					this->op1 = operand1;
					this->op2 = operand2;
					this->relOp = LT;
				}
			}
		}
	}
}

bool RangeCheckExpression::operator==(const RangeCheckExpression& other) const
{
	// cannot be equal if relops are different
	if (this->relOp != other.relOp)
	{
		return false;
	}
	else // relop same
	{
		if (dyn_cast<ConstantInt>(this->op1) && dyn_cast<ConstantInt>(other.op1)) // LHS consists of constants for both
		{
			ConstantInt* CI1 = dyn_cast<ConstantInt>(this->op1);
			ConstantInt* CI2 = dyn_cast<ConstantInt>(other.op1);

			int64_t intVal1 = CI1->getSExtValue();
			int64_t intVal2 = CI2->getSExtValue();
			
			if (!(intVal1 == intVal2))
				return false;
		}
		else if (dyn_cast<Instruction>(this->op1) && dyn_cast<Instruction>(other.op1)) // LHS consists of variables
		{
			if (!(this->op1 == other.op1))
				return false;
		}
		else
		{
			return false;
		}
		
		if (dyn_cast<Constant>(this->op2) && dyn_cast<Constant>(other.op2)) // RHS consists of constants for both
		{
			ConstantInt* CI1 = dyn_cast<ConstantInt>(this->op2);
			ConstantInt* CI2 = dyn_cast<ConstantInt>(other.op2);
				
			int64_t intVal1 = CI1->getSExtValue();
			int64_t intVal2 = CI2->getSExtValue();
			
			if (!(intVal1 == intVal2))
				return false;
		}
		else if (dyn_cast<Instruction>(this->op2) && dyn_cast<Instruction>(other.op2)) // RHS consists of variables
		{
			if (!(this->op2 == other.op2))
				return false;
		}
		else
		{
			return false;
		}
	}

	return true;
}
