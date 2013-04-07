#ifndef __LOOP_CHECK_PROPAGATION_PASS_H__
#define __LOOP_CHECK_PROPAGATION_PASS_H__

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
#include "llvm/DataLayout.h"
#include "EffectGenPass.h"
#include "ArrayBoundsCheckPass.h"

namespace llvm
{
        typedef std::vector<BasicBlock *> LoopBlocks;
        typedef SmallVector<BasicBlock *, 10> ExitingBlockVec;
	typedef std::map<BasicBlock *, Value *> BBToCheck;
	typedef std::pair<BasicBlock *, Value *> PairBBAndCheck;
        typedef enum{
                INVARIANT, INCREASING, DECREASING, WILD // TODO monotonic inc/dec?
        }effect_t;
	struct LoopCheckPropagationPass : public LoopPass {
		public:
			static char ID;
			LoopCheckPropagationPass() : LoopPass(ID) {}
                        virtual bool doInitialization (Loop *L, LPPassManager &LPM);
			virtual bool doFinalization(void);
                        virtual bool runOnLoop (Loop *L, LPPassManager &LPM);
			virtual void getAnalysisUsage(AnalysisUsage& AU) const
			{
				AU.addRequired<DominatorTree>();
				AU.addRequired<EffectGenPass>();
			}

                        void findCandidates(Loop *loop, LoopBlocks *blocks);
                        void hoist(Loop *loop, BasicBlock *block);
                        bool isCandidate(Loop *loop, Value *operandOne, Value *operandTwo);
                        bool isInvariant(Value *operand);
                        effect_t getEffect(Loop *loop, Value *operand);
		
		private:

                        BBToCheck *bbToCheck;
	};

}
#endif
