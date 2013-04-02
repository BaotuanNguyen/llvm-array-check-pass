
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
#include "ArrayBoundsCheckPass.h"
#include <set>
#include <map>
//#include "RunTimeBoundsChecking.h"




namespace llvm {

	struct GlobalOptimizationsOnArrayChecks : public FunctionPass {
		static char ID;
		public: 
			GlobalOptimizationsOnArrayChecks() : FunctionPass(ID) {}
			virtual bool doInitialization(Module &M);
			virtual bool runOnFunction(Function &F);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.addRequired<DataLayout>();
				AU.addRequired<TargetLibraryInfo>();
                                AU.addRequired<ArrayBoundsCheckPass>();
                                //LATER AU.addRequired<LocalOptimizationsOnArrayChecks>();
			}
		private:
			void findVeryBusyChecks();
			void findAvailableChecks();
			template <typename T>
				void dumpSetOfPtr(std::set<T*>* set);
			Function* currentFunction;
			typedef std::set<Value*> ValuesSet;
			typedef std::pair<BasicBlock*, ValuesSet* > PairBBToValuesSet;
			typedef std::map<BasicBlock*, ValuesSet* > MapBBToValuesSet;
	};
}

#endif /* __GLOBAL_OPTIMIZATIONS_ON_ARRAY_CHECKS_H__ */
