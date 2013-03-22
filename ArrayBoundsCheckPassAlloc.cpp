#include "ArrayBoundsCheckPass.h"

using namespace llvm;

bool ArrayBoundsCheckPass::collectVariableBeforeAlloca(AllocaInst* AI)
{
	const StringRef& variableName = AI->getName();
	Value* variable = this->createGlobalString(variableName);
	///insert the call instruction here	
	return false;
}
