
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
#include "EffectGenPass.h"
#include <set>
#include <map>
//#include "RunTimeBoundsChecking.h"

namespace llvm {

	typedef std::list<RangeCheckSet*> ListRCS;
	typedef std::map<Instruction*, RangeCheckSet*> MapInstToRCS;
	typedef std::map<BasicBlock*, RangeCheckSet*> MapBBToRCS;
	typedef std::pair<BasicBlock*, RangeCheckSet*> PairBBAndRCS;

	struct AvailableAnalysisPass : public FunctionPass {
		static char ID;
		public: 
			AvailableAnalysisPass() : FunctionPass(ID), module(NULL) {}
			virtual bool doInitialization(Module &M) {
                                this->module = &M;        
				return false;
                        }
			virtual bool runOnFunction(Function &F);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
                AU.addRequired<EffectGenPass>();
			}
			virtual bool doFinalization(Module& M);
			RangeCheckSet *getAvailOut(BasicBlock *bb, RangeCheckSet *cInOfBlock);
			void createUniverse();
		private:
			void dataFlowAnalysis(bool isForward);
			void findGenSets();
			template <typename T>
				void dumpSetOfPtr(std::set<T*>* set);

			MapInstToRCS *I_A_OUT;
			MapBBToRCS *BB_A_OUT;
			//private variables

			Module* module;
			Function* currentFunction;

			RangeCheckSet *universe;
	};
}

#endif /* __AVAILABLE_ANALYSIS_PASS_H__ */
