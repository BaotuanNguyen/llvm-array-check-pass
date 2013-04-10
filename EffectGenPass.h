#ifndef __EFFECT_GEN_PASS_H__
#define __EFFECT_GEN_PASS_H__

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Operator.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "ArrayBoundsCheckPass.h"
#include "EffectGenPass.h"
#include "RangeCheckSet.h"
#include <queue>
#include <set>
#include <map>

namespace llvm 
{
	struct EffectGenPass : public ModulePass 
	{
		public: 
			static char ID;
			EffectGenPass() : ModulePass(ID) {}
			virtual bool runOnModule(Module& M);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
			}
		private:
			bool runOnFunction(Function* func);
			bool runOnBasicBlock(BasicBlock* BB);
			void generateMetadata(MDString* str, Value* variable, Value* n, Instruction* inst, Module* M);
			Module* M;
	};
}

#endif
