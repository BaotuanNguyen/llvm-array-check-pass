#ifndef __RUN_TIME_BOUNDS_CHECKING_H__
#define __RUN_TIME_BOUNDS_CHECKING_H__
#include "llvm/IRBuilder.h"
#include "llvm/Intrinsics.h"
#include "llvm/Pass.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/MemoryBuiltins.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/TargetFolder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/DataLayout.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Transforms/Instrumentation.h"


namespace llvm {

	typedef IRBuilder<true, TargetFolder> BuilderTy;

	struct RunTimeBoundsChecking : public FunctionPass {
		static char ID;
		public: 
			RunTimeBoundsChecking() : FunctionPass(ID), Penalty(){}
			virtual bool doInitialization(Module& M);
			virtual bool runOnFunction(Function &F);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.addRequired<DataLayout>();
				AU.addRequired<TargetLibraryInfo>();
			}
		private:
			const DataLayout *TD;
			const TargetLibraryInfo *TLI;
			ObjectSizeOffsetEvaluator *ObjSizeEval;
			BuilderTy *Builder;
			Instruction *Inst;
			BasicBlock *TrapBB;
			unsigned Penalty;
			//insert checks variables
			unsigned int checkNumber;
			Module* M;		
			Function* checkFunction;
			Function* currentFunction;
			std::vector<Constant*> gepFirstCharIndices;

			BasicBlock *getTrapBB();
			void emitBranchToTrap(Value *Cmp = 0);
			void emitBranchToTrap(Value* Offset, Value* Size, Value *Cmp = 0);
			bool computeAllocSize(Value *Ptr, APInt &Offset, Value* &OffsetValue,
					APInt &Size, Value* &SizeValue);
			bool instrument(Value *Ptr, Value *Val);
			//insert checks functions
			Constant* createGlobalString(const StringRef& str);
			bool insertCheckBeforeInstruction(Value* offset, Value* size, Instruction* I);
	};
}

#endif /*__RUN_TIME_BOUNDS_CHECKING_H__*/
