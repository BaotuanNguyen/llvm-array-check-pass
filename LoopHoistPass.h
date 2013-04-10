#ifndef __LOOP_HOIST_PASS_H__
#define __LOOP_HOIST_PASS_H__

#include "llvm/User.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopPass.h"
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

namespace llvm
{
	std::vector<CallInst*>* candidates;
	
	struct LoopHoistPass : public ModulePass 
	{
		public:
			static char ID;
			LoopHoistPass() : ModulePass(ID) {}
            virtual bool runOnModule (Module& M);
			virtual void getAnalysisUsage(AnalysisUsage& AU) const
			{
				AU.addRequired<LoopInfo>();
			}
			
			bool runOnFunction(Function* func);
			bool runOnLoop(Loop *L);

			bool isInvariant(Value* operand, Loop* L);

            void findCandidates(Loop *loop);
            void hoistTo(BasicBlock* preheader);
		
		private:
				Module* M;
				LoopInfo* loopInfo;
				int numHoisted;
	};

}
#endif
