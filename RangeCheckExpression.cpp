#include "RangeCheckExpression.h"

using namespace llvm;


// checks whether we can 100% be sure about relative ordering of var1 and var2
// each variable can be in 4 conditions, which mean we need to consider 4x4=16 cases
// 1: v is a constant
// 2: v is a named variable
// 3: v is a temporary variable in a form of v = v + 4
// 4: v is a temporary variable in a form of v = v1 + v2
VALUE_STATUS RangeCheckExpression::compare(Value* var1, Value* var2)
{
//	errs() << "Comparing: " << *var1 << " to " << *var2 << "\n";
	
	if (var1 == var2)
	{
		return EQ; // easiest case
	}

	if (ConstantInt* ai = dyn_cast<ConstantInt>(var1)) // v1 is a constant
	{
//		errs() << "v1 is constant\n";

		if(ConstantInt* ci = dyn_cast<ConstantInt>(var2))
		{
//			errs() << "v2 is constant\n";
			int64_t a = ai->getSExtValue();
			int64_t c = ci->getSExtValue();
			
			if (a < c)
			{
//				errs() << "v1 < v2\n";
				return LT;
			}
			else if (a == c)
			{
//				errs() << "v1 == v2\n";
				return EQ;
			}
			else
			{
//				errs() << "v1 > v2\n";
				return GT;
			}
		}
		else if (dyn_cast<Instruction>(var2) || dyn_cast<GlobalVariable>(var2))
		{
//			errs() << "v2 is instruction\n";
			return UNKNOWN;
		}
		else
		{
			errs() << "ERROR!! UNKNOWN OPERAND!!\n";
			errs() << *var2 << "\n";
			return UNKNOWN;
		}
	}
	else if (ConstantInt* ci = dyn_cast<ConstantInt>(var2))
	{
//		errs() << "v2 is constant\n";

		if(ConstantInt* ai = dyn_cast<ConstantInt>(var1))
		{
//			errs() << "v1 is constant\n";
			int64_t a = ai->getSExtValue();
			int64_t c = ci->getSExtValue();
			
			if (a < c)
			{
//				errs() << "v1 < v2\n";
				return LT;
			}
			else if (a == c)
			{
//				errs() << "v1 == v2\n";
				return EQ;
			}
			else
			{
//				errs() << "v1 > v2\n";
				return GT;
			}
		}
		else if (dyn_cast<Instruction>(var1) || dyn_cast<GlobalVariable>(var1))
		{
//			errs() << "v2 is instruction\n";
			return UNKNOWN;
		}
		else
		{
			errs() << "ERROR!! UNKNOWN OPERAND!!\n";
			errs() << *var1 << "\n";
			return UNKNOWN;
		}
	}

	// can't deal with global variables...
	if (dyn_cast<GlobalVariable>(var1) || dyn_cast<GlobalVariable>(var2))
		return UNKNOWN;

	MDNode* metadata1 = dyn_cast<Instruction>(var1)->getMetadata("EFFECT");
	MDNode* metadata2 = dyn_cast<Instruction>(var2)->getMetadata("EFFECT");

	if (metadata1 == NULL || metadata2 == NULL)
	{
		return UNKNOWN;
	}

	if (dyn_cast<MDString>(metadata1->getOperand(0))->getString().equals("UNCHANGED")) // v1 is a named variable
	{
		Value* av = metadata1->getOperand(1);

		if (dyn_cast<MDString>(metadata2->getOperand(0))->getString().equals("UNCHANGED")) // v2 is a named variable
		{
//			errs() << "v2 is a named variable\n";
			Value* cv = metadata2->getOperand(1);

			if (av == cv)
			{
				return EQ;
			}
			else
			{
				return UNKNOWN;
			}

		}
		else if (dyn_cast<MDString>(metadata2->getOperand(0))->getString().equals("CHANGED")) // v2 unknown
		{
			return UNKNOWN;
		}
		else // v2 is a temporary variable	
		{
//			errs() << "v2 is a temporary variable\n";

			Value* cv = metadata2->getOperand(1);
			
			if (av != cv) // "related" variable in temporary c must be same to a
			{
				return UNKNOWN;
			}
			else
			{   
				// a < a+c for all c > 0
				if (dyn_cast<MDString>(metadata2->getOperand(0))->getString().equals("INCREMENT"))
					return LT;
				else if (dyn_cast<MDString>(metadata2->getOperand(0))->getString().equals("UNCHANGED"))
					return EQ;
				else
					return GT;
			}
			
		}
	}
	else // a is a temporary variable
	{
		MDString* str1 = dyn_cast<MDString>(metadata1->getOperand(0));
		MDString* str2 = dyn_cast<MDString>(metadata2->getOperand(0));

		if (str1->getString().equals("CHANGED"))
			return UNKNOWN;

		if (str2->getString().equals("CHANGED"))
			return UNKNOWN;

		Value* av = metadata1->getOperand(1);
		Value* cv = metadata2->getOperand(1);

		if (av != cv)
			return UNKNOWN;

		ConstantInt* CI1 = dyn_cast<ConstantInt>(metadata1->getOperand(2));
		ConstantInt* CI2 = dyn_cast<ConstantInt>(metadata2->getOperand(2));

		int64_t c1 = CI1->getSExtValue();
		int64_t c2 = CI2->getSExtValue();

		if (c1 < c2)
			return LT;
		else if (c1 == c2)
			return EQ;
		else
			return GT;
	}
	
	return UNKNOWN;
}

// checks whether (a<b) subsumes (c<d)
// (a<b) subsumes (c<d) if a >= c and b <= d
// returns false if it is unknown
bool RangeCheckExpression::subsumes(RangeCheckExpression* expr)
{
	Value* a = this->left;
	Value* b = this->right;
	Value* c = expr->left;
	Value* d = expr->right;

	VALUE_STATUS compare_a_c = compare(a,c);
	VALUE_STATUS compare_b_d = compare(b,d);

	if ((compare_a_c == EQ) && (compare_b_d == EQ))
	{
//		this->print(); errs() << " EQUALS "; expr->println();
		return true;
	}

	if ((compare_a_c == GT || compare_a_c == EQ) && (compare_b_d == LT || compare_b_d == EQ))
	{
//		this->print(); errs() << " SUBSUMES "; expr->println();
		return true;
	}
	else
	{
//		this->print(); errs() << " DOES NOT SUBSUME "; expr->println();
		return false;
	}
}
		
void RangeCheckExpression::print()
{
	if (ConstantInt* CI = dyn_cast<ConstantInt>(left))
	{
		errs() << CI->getSExtValue();															
	}
	else
	{
		if (!(left->hasName()))
			errs() << "temp";
		else
			errs() << left->getName().str();
	}
	
	errs() << " < ";

	if (ConstantInt* CI = dyn_cast<ConstantInt>(right))
	{
		errs() << CI->getSExtValue();															
	}
	else
	{
		if (!(right->hasName()))
			errs() << "temp";
		else
			errs() << right->getName().str();
	}
}

void RangeCheckExpression::println()
{
	if (ConstantInt* CI = dyn_cast<ConstantInt>(left))
	{
		errs() << CI->getSExtValue();															
	}
	else
	{
		if (!(left->hasName()))
		{
			errs() << "temp";
		}
		else
		{
			errs() << left->getName().str();
		}
	}
	
	errs() << " < ";

	if (ConstantInt* CI = dyn_cast<ConstantInt>(right))
	{
		errs() << CI->getSExtValue();															
	}
	else
	{
		if (!(right->hasName()))
			errs() << "temp";
		else
			errs() << right->getName().str();
	}

	errs() << "\n";
}

RangeCheckExpression::RangeCheckExpression(CallInst* inst, Module* M)
{
	this->origin = inst;

	StringRef funcName = inst->getCalledFunction()->getName();
	LLVMContext& context = M->getContext();

	if (!funcName.equals("checkLessThan"))
	{
		errs() << "ERROR! Can only create RangeCheckExpression from checkLTLimit call or checkGTLimit call\n";
	}
	
	MDNode* metadata = inst->getMetadata("VarName");
	
//	errs() << "func name " <<  *inst << "\n";
	
	if (metadata == NULL)
	{
		errs() << "ERROR! range check call does not have VarName metadata!\n";
		errs() << "You should generate effect first in order to create range check expressions!\n";
	
		this->left = NULL;
		this->right = NULL;;
		return;
	}

	Value* operand1 = metadata->getOperand(0);
	Value* operand2 = metadata->getOperand(1);

	// tries to restructure range structures such as n+1 < 5 into n < 4
	// this transformation can only work with combination of non-changed temporary variable + a constant
	if (ConstantInt* CI1 = dyn_cast<ConstantInt>(operand1))
	{
		if (!(dyn_cast<AllocaInst>(operand2)) && !(dyn_cast<GlobalVariable>(operand2)))
		{
			if (Instruction* op2inst = dyn_cast<Instruction>(operand2)) // op2 is a temporary variable
			{
					MDNode* effects = op2inst->getMetadata("EFFECT");
			
					if (effects == NULL)
					{
						this->left = operand1;
						this->right = operand2;
						return;
					}
					MDString* mdstr = dyn_cast<MDString>(effects->getOperand(0));
		
					if (!(mdstr->getString().equals("CHANGED")))
					{
						// transforms 3 < n + 1 into 2 < n
						int64_t leftVal = CI1->getSExtValue();
						ConstantInt* addInt = dyn_cast<ConstantInt>(effects->getOperand(2));
						int64_t addIntVal = addInt->getSExtValue();
						int64_t newOp1Val = leftVal - addIntVal;
		
						ConstantInt* newVal = ConstantInt::get(Type::getInt64Ty(context), newOp1Val);
						
						this->left = newVal;
						this->right = effects->getOperand(1);
						return;
					}
			}
		}
	}
	else if (ConstantInt* CI2 = dyn_cast<ConstantInt>(operand2))
	{
		if (!(dyn_cast<AllocaInst>(operand1)) && !(dyn_cast<GlobalVariable>(operand1)))
		{
			if (Instruction* op1inst = dyn_cast<Instruction>(operand1)) // op1 is a temporary variable
			{
					MDNode* effects = op1inst->getMetadata("EFFECT");

					if (effects == NULL)
					{
						this->left = operand1;
						this->right = operand2;
						return;
					}

					MDString* mdstr = dyn_cast<MDString>(effects->getOperand(0));
		
					if (!(mdstr->getString().equals("CHANGED")))
					{
						// transforms n+1 < 3 into n < 2
						int64_t rightVal = CI2->getSExtValue();
						ConstantInt* addInt = dyn_cast<ConstantInt>(effects->getOperand(2));
						int64_t addIntVal = addInt->getSExtValue();
						int64_t newOp2Val = rightVal - addIntVal;
		
						ConstantInt* newVal = ConstantInt::get(Type::getInt64Ty(context), newOp2Val);
						
						this->left = effects->getOperand(1);
						this->right = newVal;
						return;
					}
			}
		}
	}
	
	this->left = operand1;
	this->right = operand2;
}

bool RangeCheckExpression::operator==(const RangeCheckExpression& other) const
{
	RangeCheckExpression* op1 = (RangeCheckExpression*) this;
	RangeCheckExpression* op2 = (RangeCheckExpression*) &other;
	
	if (op1->subsumes(op2) && op2->subsumes(op1))
		return true;
	else
		return false;
}
