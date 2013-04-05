#include "RangeCheckExpression.h"
#include <set>
#include "llvm/InstrTypes.h"

using namespace llvm;

#ifndef __RANGE_CHECK_SET__
#define __RANGE_CHECK_SET__

class RangeCheckSet
{
	std::vector<RangeCheckExpression>* checkSet;

	public:
		RangeCheckSet()
		{
			this->checkSet = new std::vector<RangeCheckExpression>();
		}

		void insert(RangeCheckExpression expr)
		{
			this->checkSet->push_back(expr);
			std::vector<RangeCheckExpression>::iterator it;
			it = std::unique(checkSet->begin(), checkSet->end());
			checkSet->resize(std::distance(checkSet->begin(),it));
		}
		void kill_forward(Instruction* var);
		void kill_backward(Instruction* var);
		RangeCheckSet* set_union(RangeCheckSet* s);
		RangeCheckSet* set_intersect(RangeCheckSet* s);
		bool doValueKillCheckForward(RangeCheckExpression* currentCheck, Value* valueBeingStored, int variablePos);
		bool doValueKillCheckBackward(RangeCheckExpression* currentCheck, Value* valueBeingStored, int variablePos);
		int size()
		{
			return checkSet->size();
		}
};

#endif
