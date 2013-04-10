#ifndef __AVAILABLE_ANALYSIS_PASS_H__
#define __AVAILABLE_ANALYSIS_PASS_H__

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CFG.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Operator.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Instructions.h"
#include "RangeCheckSet.h"
#include <queue>
#include <set>
#include <map>

using namespace llvm;

namespace llvm {

	typedef std::list<RangeCheckSet*> ListRCS;
	typedef std::map<BasicBlock*, RangeCheckSet*> MapBBToRCS;
	typedef std::map<Instruction*, RangeCheckSet*> MapInstToRCS;
	typedef std::pair<BasicBlock*, RangeCheckSet*> PairBBAndRCS;
	typedef std::pair<Instruction*, RangeCheckSet*> PairIAndRCS;

	struct AvailableAnalysisPass : public ModulePass {
		static char ID;
		public: 
			AvailableAnalysisPass() : ModulePass(ID){}
			
			virtual bool runOnModule(Module& M);
			
			virtual bool doInitialization(Module &M) 
			{
				return false;
            }

			bool runOnFunction(Function *F);
			virtual void getAnalysisUsage(AnalysisUsage& AU) const
			{
//				AU.addRequired<EffectGenPass>();
			}
			RangeCheckSet *getAvailOut(BasicBlock *bb, RangeCheckSet *cInOfBlock);
			void createUniverse();
			MapInstToRCS *I_A_OUT;
		
		private:
			void dataFlowAnalysis();
			void findGenSets();

			MapBBToRCS *BB_A_OUT;

			Module* module;
			Function* currentFunction;

			RangeCheckSet *universe;
	};
}

#endif
