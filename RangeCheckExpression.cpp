#include <string>

#ifndef __RANGE_CHECK_EXPRESSION__
#define __RANGE_CHECK_EXPRESSION__

enum RelOps
{
	GTEQ, LT
}

class RangeCheckExpression
{
	string var1;
	string var2;
	RelOps relOp;

	public RangeCheckExpression(string var1, string var2, RelOps relOp)
	{
		this->var1 = var1;
		this->var2 = var2;
		this->relOp = relOp;
	}

	public relOp getRelOp()
	{
		return relOp;
	}

	public boolean subsumes(RangeCheckExpression* expr)
	{
		if (this->relOp != expr->getRelOp())
		{
			return false;
		}
	}
}

#endif
