#ifndef __LOOP_HOIST_PASS_H__
#define __LOOP_HOIST_PASS_H__

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Operator.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/LoopPass.h"
#include "ArrayBoundsCheckPass.h"
#include "RangeCheckSet.h"
#include <queue>
#include <set>
#include <sstream>

using namespace llvm;

namespace llvm
{

        typedef enum{
                WILD, INVARIANT, INCREASING, DECREASING
        }effect_t;

        typedef std::vector<BasicBlock *> LoopBlocks;
        typedef SmallVector<BasicBlock *, 10> ExitingBlockVec;
        typedef std::set<Instruction *> CheckSet;
	typedef std::map<BasicBlock *, CheckSet *> BBToCheckSet;
	typedef std::pair<BasicBlock *, CheckSet *> PairBBAndCheckSet;
	typedef std::pair<BasicBlock *, Instruction *> PairBBAndInst;
	typedef std::vector<PairBBAndInst *> BBAndInstVec;
        typedef std::vector<Instruction *> MoveVec;
	typedef std::pair<std::string, Value *> NameAndValuePair;
        typedef std::map<Value *, NameAndValuePair *> ValueToNameAndValuePair; // map old names to pair of new name and value
        typedef std::pair<Value *, NameAndValuePair *> OldAndNewPair;
        typedef std::map<CallInst *, effect_t> CheckToCandidacy;
        typedef std::pair<CallInst *, effect_t> CheckCandidacyPair;

	struct LoopHoistPass : public LoopPass {
		public:
			static char ID;
			LoopHoistPass() : LoopPass(ID) {}
                        virtual bool doInitialization (Loop *L, LPPassManager &LPM);
			virtual bool doFinalization(void);
                        virtual bool runOnLoop (Loop *L, LPPassManager &LPM);
			virtual void getAnalysisUsage(AnalysisUsage& AU) const
			{
				AU.addRequired<DominatorTree>();
//				AU.addRequired<EffectGenPass>();
			}

                        void findCandidates(Loop *loop);
                        void prepHoist(Loop *loop);
                        void hoist(Loop *loop);
                        void addDependencies(Loop *loop, MoveVec *moveVec, Value *v);
                        effect_t isCandidate(Loop *loop, Value *operandOne, Value *operandTwo);
                        effect_t getEffect(Loop *loop, Value *operand, int change);
                        Value *swapFakeOperand(Value *operand);

                        std::string getEffectOfMeta(MDNode *meta);
                        Value *getAffectedOperandOfMeta(MDNode *meta);
		
		private:

                        BBToCheckSet *bbToCheck;
                        BBAndInstVec *bbAndInstVec;
                        ValueToNameAndValuePair *oldToNew;
                        CheckToCandidacy *checkToCandidacy;
	};

}
#endif
