#ifndef __MODIFY_CHECK_PASS_H__
#define __MODIFY_CHECK_PASS_H__

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Operator.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Instructions.h"
#include "VeryBusyAnalysisPass.h"
#include "EffectGenPass.h"
#include "RangeCheckSet.h"
#include <queue>
#include <set>
#include <map>

using namespace llvm;

namespace llvm 
{
	typedef std::map<Instruction*, RangeCheckSet*> MapInstToRCS;
	
	struct ModifyCheckPass : public ModulePass 
	{
		public: 
			static char ID;
			ModifyCheckPass() : ModulePass(ID) {}
			virtual bool runOnModule(Module& M);
			void modify(CallInst* callInst, RangeCheckSet* RCS, Module* M);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const 
			{
				AU.addRequired<VeryBusyAnalysisPass>();
			}
		private:
			bool runOnFunction(Function* func);
			bool runOnBasicBlock(BasicBlock* BB);
			Module* M;
			VeryBusyAnalysisPass* veryBusyAnalysis;
			MapInstToRCS* veryBusyMap;
			void generateMetadata(MDString* str, Value* variable, Value* n, Instruction* inst, Module* M);
	};
}

#endif 
