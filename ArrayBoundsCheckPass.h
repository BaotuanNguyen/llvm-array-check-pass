
#ifndef __ARRAY_BOUNDS_CHECK_PASS_H__
#define __ARRAY_BOUNDS_CHECK_PASS_H__
#include "llvm/User.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/InstrTypes.h"
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
		Value* createGlobalString(const StringRef& str);
		bool findArrayAccess(Function& F);
		///defined in their respective files
		bool insertCheckBeforeAccess(GetElementPtrInst* GEP);
		bool collectVariableBeforeAlloca(AllocaInst* AI);
		///everything else defined in main array checks file
		Value* findOriginOfPointer(Value* pointer);
		virtual void getAnalysisUsage(AnalysisUsage& AU) const
		{
			AU.addRequired<DataLayout>();
			AU.addRequired<TargetLibraryInfo>();
		}
	private:
		unsigned int numBlockVisited;
		///this is the function currently being visited
		Module* M;
		Function* currentFunction;
		Function* arrayAccessFunction;
		Function* allocaFunction;
};

}

#endif
