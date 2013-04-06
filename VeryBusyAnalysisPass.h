#ifndef __VERY_BUSY_ANALYSIS_PASS_H__
#define __VERY_BUSY_ANALYSIS_PASS_H__

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
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/DataLayout.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "ArrayBoundsCheckPass.h"
#include "EffectGenPass.h"
#include "RangeCheckSet.h"
#include <set>
#include <map>

namespace llvm {

	typedef std::list<RangeCheckSet*> ListRCS;
	typedef std::map<Instruction*, RangeCheckSet*> MapInstToRCS;
	typedef std::map<BasicBlock*, RangeCheckSet*> MapBBToRCS;
	typedef std::pair<Instruction*, RangeCheckSet*> PairIAndRCS;
	typedef std::pair<BasicBlock*, RangeCheckSet*> PairBBAndRCS;

	struct VeryBusyAnalysisPass : public ModulePass {
		static char ID;
		public: 
			VeryBusyAnalysisPass() : ModulePass(ID){} 
			
			virtual bool runOnModule(Module& M);
			bool runOnFunction(Function *F);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const 
			{
                                AU.addRequired<EffectGenPass>();
			}
			RangeCheckSet *getVBIn(BasicBlock *bb, RangeCheckSet *cOutOfBlock);
			void createUniverse();
			MapInstToRCS *I_VB_IN;
		
		private:
			void dataFlowAnalysis();
			void findGenSets();
			template <typename T>
				void dumpSetOfPtr(std::set<T*>* set);

			MapBBToRCS *BB_VB_IN;

			Module* module;
			Function* currentFunction;
			RangeCheckSet *universe;
	};
}

#endif /* __VERY_BUSY_ANALYSIS_PASS_H__ */
