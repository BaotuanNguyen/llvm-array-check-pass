#include "RangeCheckExpression.h"
#include <set>
#include "llvm/InstrTypes.h"

using namespace llvm;

class RangeCheckSet
{
	std::vector<RangeCheckExpression>* checkSet;

	public:
		RangeCheckSet()
		{
			this->checkSet = new std::vector<RangeCheckExpression>();
		}

	private:
		void insert(RangeCheckExpression expr)
		{
			this->checkSet->push_back(expr);
			std::vector<RangeCheckExpression>::iterator it;
			checkSet->resize(std::distance(checkSet->begin(),it));
		}
		void kill_forward(Instruction* var);
		void kill_backward(Instruction* var);
		RangeCheckSet* set_union(RangeCheckSet* s);
		RangeCheckSet* set_intersect(RangeCheckSet* s);
};
