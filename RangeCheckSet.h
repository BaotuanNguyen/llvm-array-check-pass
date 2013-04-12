#ifndef __RANGE_CHECK_SET__
#define __RANGE_CHECK_SET__

#include "RangeCheckExpression.h"
#include "llvm/InstrTypes.h"
#include "RangeCheckExpression.h"
#include "RangeCheckSet.h"
#include <iterator>
#include <set>

using namespace llvm;

class RangeCheckSet
{
	public:
		std::vector<RangeCheckExpression>* checkSet;
		
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
			std::vector<RangeCheckExpression>::iterator it;

			for (it = this->checkSet->begin(); it != this->checkSet->end(); it++)
			{
				if ((*it) == expr)
					return;
			}

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

		void kill_forward(Instruction* var, Module* M);
		void kill_backward(Instruction* var, Module* M);
		RangeCheckSet* set_union(RangeCheckExpression* s);
		RangeCheckSet* set_intersect(RangeCheckSet* s);
		RangeCheckSet* set_intersect_proper(RangeCheckSet* s);
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
				int beforeSize = vecThis->size();
				if ((this->set_intersect_proper(anotherRCS))->size() == beforeSize)
					return true;
				else
					return false;

//				return std::equal(vecThis->begin(), vecThis->end(), vecThat->begin());
			}
		}

		int size()
		{
			return checkSet->size();
		}
};

#endif
