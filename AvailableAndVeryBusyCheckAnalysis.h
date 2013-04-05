
#ifndef __GLOBAL_OPTIMIZATIONS_ON_ARRAY_CHECKS_H__
#define __GLOBAL_OPTIMIZATIONS_ON_ARRAY_CHECKS_H__

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
#include "RangeCheckSet.h"
#include <set>
#include <map>
//#include "RunTimeBoundsChecking.h"




namespace llvm {

	///old definitions
	typedef std::set<Value*> ValuesSet; typedef std::pair<BasicBlock*, ValuesSet* > PairBBToValuesSet; typedef std::map<BasicBlock*, ValuesSet* > MapBBToValuesSet;
	typedef std::list<ValuesSet*> ListOfValuesSets;
	///new definitions
	typedef std::map<Instruction*, RangeCheckSet*> MapInstToRCS;
	typedef std::map<BasicBlock*, RangeCheckSet*> MapBBToRCS;

	struct AvailableAndVeryBusyCheckAnalysis : public FunctionPass {
		static char ID;
		public: 
			AvailableAndVeryBusyCheckAnalysis() : FunctionPass(ID) {}
			virtual bool doInitialization(Module &M);
			virtual bool runOnFunction(Function &F);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.addRequired<DataLayout>();
				AU.addRequired<TargetLibraryInfo>();
                                AU.addRequired<ArrayBoundsCheckPass>();
				AU.addRequired<AliasAnalysis>();
				AU.addRequired<ScalarEvolution>();
                                //LATER AU.addRequired<LocalOptimizationsOnArrayChecks>();
			}
			virtual bool doFinalization(Module& M);
			RangeCheckSet *getVBIn(BasicBlock *bb, RangeCheckSet *cOutOfBlock)
			RangeCheckSet *getAvailOut(BasicBlock *bb, RangeCheckSet *cInOfBlock)
		private:
			enum EffectTy {
				unchangedTy, incrementTy, decrementTy, multiplyTy, divIntTy, divLessIntTy, changedTy
			};
			///effect - find the effect for a given variable within a given block, this method will
			///go throught the instructions in a block, and determine how the variable is affected. 
			///Value should be either a local or global variable, else this will always return changedTy.
			///
			EffectTy effect(BasicBlock* B, Value* v);
			void dataFlowAnalysis(bool isForward);
			void findGenSets();
			template <typename T>
				void dumpSetOfPtr(std::set<T*>* set);
			Function* currentFunction;

			MapBBToValuesSet* VeryBusy_Gen;
			MapBBToValuesSet* Available_Gen;

			MapInstToRCS *I_VB_IN, *I_A_OUT;
			MapBBToRCS *BB_VB_IN, *BB_A_OUT;
			//private variables
			ScalarEvolution* SE;
			AliasAnalysis* AA;
	};
}

#endif /* __GLOBAL_OPTIMIZATIONS_ON_ARRAY_CHECKS_H__ */
