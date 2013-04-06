#include "RangeCheckExpression.h"
#include <set>
#include <iterator>
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
		
		RangeCheckSet(int n)
		{
			this->checkSet = new std::vector<RangeCheckExpression>(n);
		}

		RangeCheckSet* copy()
		{
			RangeCheckSet* newRCS = new RangeCheckSet(this->size());
			std::copy(this->checkSet->begin(), this->checkSet->end(), newRCS->checkSet->begin());
			return newRCS;
		}

		void insert(RangeCheckExpression expr)
		{
			this->checkSet->push_back(expr);
		}

		void println()
		{
			errs() << "{ ";
			for (std::vector<RangeCheckExpression>::iterator it = this->checkSet->begin(); it != this->checkSet->end(); ++it)
			{
				errs() << "(";
				(*it).print();
				errs() << ") ";
			}
			errs() << " }\n";
		}

		void kill_forward(Instruction* var);
		void kill_backward(Instruction* var);
		RangeCheckSet* set_union(RangeCheckExpression* s);
		RangeCheckSet* set_intersect(RangeCheckSet* s);
		bool doValueKillCheckForward(RangeCheckExpression* currentCheck, Value* valueBeingStored, int variablePos);
		bool doValueKillCheckBackward(RangeCheckExpression* currentCheck, Value* valueBeingStored, int variablePos);
		
		bool equal(RangeCheckSet* anotherRCS)
		{
			std::vector<RangeCheckExpression> *vecThis = this->checkSet, *vecThat = anotherRCS->checkSet;
			if(vecThis->size() != vecThat->size()){
				return false;
			}
			else if (vecThis->size() == 0)
			{
				return true;
			}
			else
			{
				return std::equal(vecThis->begin(), vecThis->end(), vecThat->begin());
			}
		}

		int size()
		{
			return checkSet->size();
		}
};

#endif
