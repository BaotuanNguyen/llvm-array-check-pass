#ifndef __EFFECT_GEN_PASS_H__
#define __EFFECT_GEN_PASS_H__

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
#include <tr1/unordered_map>
//#include "RunTimeBoundsChecking.h"

namespace llvm 
{
	struct EffectGenPass : public ModulePass 
	{
		public: 
			static char ID;
			EffectGenPass() : ModulePass(ID) {}
			virtual bool runOnModule(Module& M);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.addRequired<ArrayBoundsCheckPass>();
			}
		private:
			bool runOnFunction(Function* func);
			bool runOnBasicBlock(BasicBlock* BB);
			void generateMetadata(MDString* str, Value* variable, Value* n, Instruction* inst, Module* M);
			Module* M;
	};
}

#endif /* __LOCAL_OPTIMIZATIONS_ON_ARRAY_CHECKS_H__ */
