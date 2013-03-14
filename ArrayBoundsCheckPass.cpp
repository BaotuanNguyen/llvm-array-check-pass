
#include "llvm/User.h"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "ArrayBoundsCheckPass.h"

using namespace llvm;

char ArrayBoundsCheckPass::ID = 0;
static RegisterPass<ArrayBoundsCheckPass> X("hello", "hello pass", false, false);

bool ArrayBoundsCheckPass::runOnFunction(Function& F)
{
	errs() << F.getName() << "\n";

	for(inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++)
	{
		Instruction* instruction = &(*I);
		//GetElementPtrInst* getInstruction = this->findGetElementPtrInst(instruction);
		//errs() << ((getInstruction != NULL) ? "this is element ptr instruction\n" : "");
		errs() << *instruction << "\n";
	}

	return false;

}

GetElementPtrInst* ArrayBoundsCheckPass::findGetElementPtrInst(Instruction* instruction)
{
	if(GetElementPtrInst* gepInstruction = dyn_cast<GetElementPtrInst>(instruction))
	{
		//the instruction is a GEP instruction so return it
		return gepInstruction;
	}
	else
	{
		//check whether a GEP instruction is an argument to an instruction
		errs() << "[ ";
		for(llvm::User::value_op_iterator childValuesIterator = instruction->value_op_begin(); childValuesIterator != instruction->value_op_end(); childValuesIterator++)
		{
			Value* value = *childValuesIterator;
			errs() << *value << ", ";
			if(GetElementPtrInst* childGepInstruction = dyn_cast<GetElementPtrInst>(value))
			{
				return childGepInstruction;
			}
		}
		errs() << "]\n";
	}
	return (GetElementPtrInst*)0;

}

