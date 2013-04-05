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
	struct ArrayBoundsCheckPass : public ModulePass
	{
		public:
			static char ID;
			ArrayBoundsCheckPass() : ModulePass(ID) {}
			virtual bool doInitialization(Module& M);
			virtual bool doFinalization(Module& M);
			virtual bool runOnModule(Module& F);
			Value* findOriginOfPointer(Value* pointer);
			virtual void getAnalysisUsage(AnalysisUsage& AU) const
			{
				AU.addRequired<DataLayout>();
				AU.addRequired<TargetLibraryInfo>();
                //AU.addRequired<RunTimeBoundsChecking>();
			}
		private:
			/*check insertion functions*/
			//Constant* createGlobalString(const StringRef* str);
			Instruction* checkGTZero(Value* index);
			Instruction* checkLTLimit(Value* index, Value* limit);
			Instruction* insertGTZeroCheck(Value* index);
			Instruction* insertLTLimitCheck(Value* index, Value* limit);
			/*gep checker functions*/
			bool checkGEP(User* GEP, Instruction* currInst);
			bool runOnInstruction(Instruction* inst);
			bool runOnConstantExpression(ConstantExpr* CE, Instruction* currInst);
			bool runOnFunction(Function* F);

			unsigned int checkNumber;
			Module* M;
			/*current function being checked*/
			Function* currentFunction;
			/*check function declared for check insertion*/
			Function* checkGTZeroFunction;
			Function* checkLTLimitFunction;
			/*instruction before which to insert instructions*/
			Instruction* Inst;
			std::vector<Constant*> gepFirstCharIndices;
	};

}
#endif
