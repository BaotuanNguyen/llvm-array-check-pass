
#ifndef __ARRAY_BOUNDS_CHECK_PASS_H__
#define __ARRAY_BOUNDS_CHECK_PASS_H__

namespace llvm
{

struct ArrayBoundsCheckPass : public FunctionPass
{
	public:
		static char ID;
		ArrayBoundsCheckPass() : FunctionPass(ID) {}
		virtual bool runOnFunction(Function& F);
		bool linearizeAllInstructions(Function& F);
		bool linearizeInstruction(Instruction* instruction);
};

}

#endif
