#ifndef __RANGE_CHECK_EXPRESSION__
#define __RANGE_CHECK_EXPRESSION__

#include "ArrayBoundsCheckPass.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"
#include <string>
#include "ArrayBoundsCheckPass.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"

using namespace llvm;

enum VALUE_STATUS
{
	UNKNOWN, LT, EQ, GT
};

class RangeCheckExpression
{
	public:
		Value* left;
		Value* right;
		CallInst* origin; // instruction where this RCE originated from

		RangeCheckExpression()
		{
			this->left = NULL;
			this->right = NULL;
			this->origin = NULL;
		}
		
		RangeCheckExpression(CallInst* Inst, Module* M);
		
		bool operator==(const RangeCheckExpression& other) const;
		
		void print();
		void println();

		VALUE_STATUS compare(Value* var1, Value* var2);
		bool subsumes(RangeCheckExpression* expr);
};

#endif
