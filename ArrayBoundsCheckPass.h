
#ifndef __ARRAY_BOUNDS_CHECK_PASS_H__
#define __ARRAY_BOUNDS_CHECK_PASS_H__
#include "llvm/User.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Operator.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
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
		///defined in their respective files
		bool insertCheckBeforeAccess(GetElementPtrInst* GEP);
		
		///everything else defined in main array checks file
		Value* findOriginOfPointer(Value* pointer);
		virtual void getAnalysisUsage(AnalysisUsage& AU) const
		{
			AU.addRequired<DataLayout>();
			AU.addRequired<TargetLibraryInfo>();
		}
	private:
		bool checkGEPExpression(ConstantExpr* GEP, Instruction* currInst);
		bool checkGEPInstruction(Instruction* GEPInst);
		bool runOnInstruction(Instruction* inst);
		bool runOnConstantExpression(ConstantExpr* CE, Instruction* currInst);
		void die();
		unsigned int numBlockVisited;
		Module* M;		
		Function* dieFunction;
};

}

#endif
