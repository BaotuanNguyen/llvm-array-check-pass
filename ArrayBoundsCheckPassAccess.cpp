
#include "ArrayBoundsCheckPass.h"


using namespace llvm;

bool ArrayBoundsCheckPass::insertCheckBeforeAccess(GetElementPtrInst* GEP)
{
	errs() << "call inserted\n";
	Value* ten = ConstantInt::get(Type::getInt32Ty(this->currentFunction->getContext()), 10);
	CallInst::Create(this->arrayAccessFunction, ten, "", GEP);
	this->insertCheckBeforeAccess(GEP);
	return false;
}
