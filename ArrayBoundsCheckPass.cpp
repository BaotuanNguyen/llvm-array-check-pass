
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

	this->linearizeAllInstructions(F);	
	return true;

}

bool ArrayBoundsCheckPass::linearizeAllInstructions(Function& F)
{
	errs() << F.getName() << "\n";
	//iterator over instructions
	for(inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++)
	{
		Instruction* instruction = &(*I);
		//linearize this insturction
		this->linearizeInstruction(instruction);
		//errs() << ((getInstruction != NULL) ? "this is element ptr instruction\n" : "");
	}
	return false;
}
/*
 * whether the instruction was linearized
 */
bool ArrayBoundsCheckPass::linearizeInstruction(Instruction* instruction)
{
	unsigned int index;
	errs() << "-> ("<< instruction->mayReadOrWriteMemory() <<", " << instruction->getOpcodeName() << ") " << *instruction;
	errs() << "[ ";
	for(index = 0; index < instruction->getNumOperands(); index++)
	{
		Value* operand = instruction->getOperand(index);
		errs() << *operand << ", ";
	}
	errs() << " ]\n";
	return true;;

}

