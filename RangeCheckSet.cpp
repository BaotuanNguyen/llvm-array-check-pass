#include "RangeCheckExpression.h"
#include "RangeCheckSet.h"
#include <set>
#include "llvm/InstrTypes.h"

using namespace llvm;
		
RangeCheckSet* RangeCheckSet::set_union(RangeCheckSet* s)
{
	return NULL;
}

RangeCheckSet* RangeCheckSet::set_intersect(RangeCheckSet* s)
{
	return NULL;
}

void RangeCheckSet::kill_forward(Instruction* var)
{
	return;
}

void RangeCheckSet::kill_backward(Instruction* var)
{
	return;
}
