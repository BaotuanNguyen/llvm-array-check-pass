
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
#include "RunTimeBoundsChecking.h"

namespace llvm
{

	struct ArrayBoundsCheckPass : public FunctionPass
	{
		public:
			static char ID;
			ArrayBoundsCheckPass() : FunctionPass(ID) {}
			virtual bool doInitialization(Module& M);
			virtual bool runOnFunction(Function& F);
			Constant* createGlobalString(const StringRef& str);
			bool insertCheckBeforeInstruction(Instruction* I);
			Value* findOriginOfPointer(Value* pointer);
			virtual void getAnalysisUsage(AnalysisUsage& AU) const
			{
				AU.addRequired<DataLayout>();
				AU.addRequired<TargetLibraryInfo>();
				AU.addRequired<RunTimeBoundsChecking>();
			}
		private:
			bool checkGEP(User* GEP, Instruction* currInst);
			bool runOnInstruction(Instruction* inst);
			bool runOnConstantExpression(ConstantExpr* CE, Instruction* currInst);
			void die();
			unsigned int checkNumber;
			Module* M;		
			Function* currentFunction;
			Function* checkFunction;
			Function* dieFunction;
			std::vector<Constant*> gepFirstCharIndices;
	};

}

#endif
