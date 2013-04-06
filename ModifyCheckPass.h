#ifndef __MODIFY_CHECK_PASS_H__
#define __MODIFY_CHECK_PASS_H__

#include "EffectGenPass.h"
#include "llvm/User.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Operator.h"
#include "llvm/ADT/DenseMap.h"
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
#include "llvm/Analysis/Dominators.h"
#include "ArrayBoundsCheckPass.h"
#include "VeryBusyAnalysisPass.h"

namespace llvm 
{
	struct ModifyCheckPass : public ModulePass 
	{
		public: 
			static char ID;
			ModifyCheckPass() : ModulePass(ID) {}
			virtual bool runOnModule(Module& M);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const 
			{
				AU.addRequired<VeryBusyAnalysisPass>();
			}
		private:
			bool runOnFunction(Function* func);
			bool runOnBasicBlock(BasicBlock* BB);
			Module* M;
	};
}

#endif 
