
#ifndef __LOCAL_OPTIMIZATIONS_ON_ARRAY_CHECKS_H__
#define __LOCAL_OPTIMIZATIONS_ON_ARRAY_CHECKS_H__

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
#include <tr1/unordered_map>
//#include "RunTimeBoundsChecking.h"




namespace llvm {

        typedef std::tr1::unordered_map<MDNode *, MDNode *> LocalTable;

	struct LocalOptimizationsOnArrayChecks : public BasicBlockPass {
		static char ID;
		public: 
			LocalOptimizationsOnArrayChecks() : BasicBlockPass(ID) {}
			virtual bool doInitialization(Module &M);
			virtual bool doInitialization(Function &F);
			virtual bool runOnBasicBlock(BasicBlock &BB);
			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.addRequired<DataLayout>();
				AU.addRequired<TargetLibraryInfo>();
                                AU.addRequired<ArrayBoundsCheckPass>();
			}
                        //void vtInsert(std::tr1::unordered_map<Instruction *, int> &table, MDNode *key);
                        void vtInsert(LocalTable &table, MDNode *key);
		private:
	};
}

#endif /* __LOCAL_OPTIMIZATIONS_ON_ARRAY_CHECKS_H__ */
