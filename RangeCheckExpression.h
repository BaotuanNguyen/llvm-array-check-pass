#include "ArrayBoundsCheckPass.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"
#include <string>

#ifndef __RANGE_CHECK_EXPRESSION__
#define __RANGE_CHECK_EXPRESSION__

using namespace llvm;

enum RelOps
{
	GTEQ, LT
};

class RangeCheckExpression
{
	public:
		Value* op1;
		Value* op2;
		RelOps relOp;

		RangeCheckExpression()
		{
			this->op1 = NULL;
			this->op2 = NULL;
			this->relOp = LT;
		}
		
		RangeCheckExpression(Value* op1, RelOps relOp, Value* op2)
		{
			this->op1 = op1;
			this->op2 = op2;
			this->relOp = relOp;
		}

		RangeCheckExpression(CallInst* Inst, Module* M);
		
		bool operator==(const RangeCheckExpression& other) const;
		
		void print();
		void println();

		RelOps getRelOp()
		{
			return relOp;
		}

		bool subsumes(RangeCheckExpression* expr);
};

#endif
