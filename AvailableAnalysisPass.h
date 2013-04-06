#ifndef __AVAILABLE_ANALYSIS_PASS_H__
#define __AVAILABLE_ANALYSIS_PASS_H__

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
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "ArrayBoundsCheckPass.h"
#include "EffectGenPass.h"
#include "RangeCheckSet.h"
#include "ModifyCheckPass.h"
#include <set>
#include <map>
//#include "RunTimeBoundsChecking.h"

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
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.addRequired<ModifyCheckPass>();
			}
			RangeCheckSet *getAvailOut(BasicBlock *bb, RangeCheckSet *cInOfBlock);
			void createUniverse();
			MapInstToRCS *I_A_OUT;
		
		private:
			void dataFlowAnalysis();
			void findGenSets();
			template <typename T>
				void dumpSetOfPtr(std::set<T*>* set);

			MapBBToRCS *BB_A_OUT;
			//private variables

			Module* module;
			Function* currentFunction;

			RangeCheckSet *universe;
	};
}

#endif /* __AVAILABLE_ANALYSIS_PASS_H__ */
