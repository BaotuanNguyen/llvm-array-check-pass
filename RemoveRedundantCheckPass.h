#ifndef __REMOVE_REDUNDANT_CHECK_PASS_H__
#define __REMOVE_REDUNDANT_CHECK_PASS_H__

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Operator.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Instructions.h"
#include "AvailableAnalysisPass.h"
#include "EffectGen.h"
#include "RangeCheckSet.h"
#include <queue>
#include <set>
#include <map>
#include <vector>

using namespace llvm;

namespace llvm 
{
	typedef std::map<Instruction*, RangeCheckSet*> MapInstToRCS;
	
	struct RemoveRedundantCheckPass : public ModulePass 
	{
		public: 
			static char ID;
			RemoveRedundantCheckPass() : ModulePass(ID) {}
			virtual bool runOnModule(Module& M);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const 
			{
				AU.addRequired<AvailableAnalysisPass>();
			}
		private:
			bool runOnFunction(Function* func);
			bool runOnBasicBlock(BasicBlock* BB);
			Module* M;
			AvailableAnalysisPass* availableAnalysis;
			MapInstToRCS* availableMap;
			int removedNum;
	};
}

#endif 
