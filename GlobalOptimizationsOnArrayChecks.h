
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
	};
}

#endif /* __GLOBAL_OPTIMIZATIONS_ON_ARRAY_CHECKS_H__ */
