
#ifndef __ARRAY_BOUNDS_CHECK_PASS_H__
#define __ARRAY_BOUNDS_CHECK_PASS_H__
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/DataLayout.h"
#include "llvm/Target/TargetLibraryInfo.h"

namespace llvm
{

struct FunctionGetterModulePass : public ModulePass
{
	public:
		static char ID;
		FunctionGetterModulePass() : ModulePass(ID)
		{

		}
		virtual bool runOnModule(Module& M)
		{
			this->arrayAccessFunction = M.getFunction("arrayAccess");
			return true;
		}
	private:
		Function* arrayAccessFunction;
};

struct ArrayBoundsCheckPass : public FunctionPass
{
	public:
		static char ID;
		ArrayBoundsCheckPass() : FunctionPass(ID)
		{
			this->numBlockVisited = 0;
		}
		virtual bool doInitialization(Module& M);
		virtual bool runOnFunction(Function& F);
		bool findArrayAccess(Function& F);
		Value* findOriginOfPointer(Value* pointer);
		virtual void getAnalysisUsage(AnalysisUsage& AU) const
		{
			AU.addRequired<DataLayout>();
			AU.addRequired<TargetLibraryInfo>();
		}
	private:
		unsigned int numBlockVisited;
		Function* arrayAccessFunction;
};

char ArrayBoundsCheckPass::ID = 0;
char FunctionGetterModulePass::ID = 0;

static RegisterPass<FunctionGetterModulePass> X("fgmpass", "get function getter declaration pass", false, false);
static RegisterPass<ArrayBoundsCheckPass> Y("array-check", "Array Access Checks Inserted", false, false);

}

#endif
