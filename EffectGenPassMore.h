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
	struct EffectGenPassMore : public ModulePass 
	{
		public: 
			static char ID;
			EffectGenPassMore() : ModulePass(ID) {}
			std::map<Instruction*, RangeCheckExpression*> RCEMap;	
			virtual bool runOnModule(Module& M);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
//				AU.addRequired<ArrayBoundsCheckPass>();
			}
		private:
			bool runOnFunction(Function* func);
			bool runOnBasicBlock(BasicBlock* BB);
			void generateMetadata(MDString* str, Value* variable, Value* n, Instruction* inst, Module* M);
			Module* M;
			//bool changed;
	};
}

#endif
