#include "ArrayBoundsCheckPass.h"

using namespace llvm;

bool ArrayBoundsCheckPass::collectVariableBeforeAlloca(AllocaInst* AI)
{
	const StringRef& variableName = AI->getName();
	this->createGlobalString(variableName);
	return false;
}
