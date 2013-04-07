#include "LoopCheckPropagationPass.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"
#include "stdlib.h"
#include <set>
#include <queue>

using namespace llvm;

char LoopCheckPropagationPass::ID = 0;
static RegisterPass<LoopCheckPropagationPass> Y("loop-pass", "Propagate of checks out of loops", false, false);




bool LoopCheckPropagationPass::doInitialization(Loop *L, LPPassManager &LPM)
{
        errs() << "\n";
        errs() << "\n";
        errs() << "++++++++++++++++++++++++++++++++++++++++++++\n";
        errs() << "Beginning propagation of checks out of loops\n";
        errs() << "++++++++++++++++++++++++++++++++++++++++++++\n";
        errs() << "\n";

        this->bbToCheck = new BBToCheck();

        return true;
}

bool LoopCheckPropagationPass::runOnLoop(Loop *L, LPPassManager &LPM)
{
        findCandidates(L); // populates the this->bbToCheck (mapping of basic blocks to its candidate checks)
        hoist(L);
        return true;
}

bool LoopCheckPropagationPass::doFinalization(void)
{
	return true;
}

void LoopCheckPropagationPass::findCandidates(Loop *loop)
{
        LoopBlocks *blocks = &loop->getBlocksVector();
        for(LoopBlocks::iterator it = blocks->begin(), ie = blocks->end(); it != ie; ++it){
                BasicBlock *block = *it;
                errs() << "\n--beginning new loop block--\n";
                for(BasicBlock::iterator bbIt = block->begin(), bbIe = block->end(); bbIt != bbIe; ++bbIt){
                        Value *v = &*bbIt;
                        errs() << *v << "\n";
                        if(CallInst *ci = dyn_cast<CallInst> (v)){
				const StringRef& callFunctionName = ci->getCalledFunction()->getName();
				if(callFunctionName.equals("checkLTLimit") || callFunctionName.equals("checkGTLimit")){
                                        errs() << "callinst: " << *ci << "\n";
                                        MDNode* metadata = ci->getMetadata("VarName");
                                        errs() << "metadata: " << *metadata;

                                        Value *operandOne = metadata->getOperand(0);
                                        Value *operandTwo = metadata->getOperand(1);

                                        if(isCandidate(loop, operandOne, operandTwo)){
                                                // CallInst ci is a candidate check for BasicBlock block
                                                bbToCheck->insert(PairBBAndCheck(block, ci));
                                        }
                                }
                        }
                }
        }
}




void LoopCheckPropagationPass::hoist(Loop *loop)
{
        LoopBlocks *blocks = &loop->getBlocksVector();
        for(LoopBlocks::iterator it = blocks->begin(), ie = blocks->end(); it != ie; ++it){
                BasicBlock *block = *it;

                //errs() << "hoisting\n";
                std::vector<BasicBlock *> ND; 
                // ND is the set of all blocks that do not dominate all loop exits
                // Cn is the set of all checks in n, s.t. n is an element of ND, where each candidate check will be executed in n
                //
                // first, get all loop exit blocks
                // for every block, check if it dominates all those loop exit blocks
                // if it does not, add it to ND
                DominatorTree &dt = getAnalysis<DominatorTree>();
                ExitingBlockVec loopExitingBlocks;
                loop->getExitingBlocks(loopExitingBlocks);
                unsigned int numDominated = 0;
                for(ExitingBlockVec::iterator it = loopExitingBlocks.begin(), ie = loopExitingBlocks.end(); it != ie; ++it){
                        if(dt.dominates(block, *it)){
                                ++numDominated;
                        }else{
                                break;
                        }
                }
                if(numDominated == loopExitingBlocks.size()){
                        // block does not dominate all exiting blocks
                        ND.push_back(block);
                }
        }


}



/*
 * called by findCandidates()
 * operandOne and operandTwo are the two operands of a checkLTLimit or checkGTZero call
 */
bool LoopCheckPropagationPass::isCandidate(Loop *loop, Value *operandOne, Value *operandTwo)
{
        effect_t operandOneEffect = getEffect(loop, operandOne);
        effect_t operandTwoEffect = getEffect(loop, operandTwo);
        errs() << "operandOneEffect: " << operandOneEffect << "\n";
        errs() << "operandTwoEffect: " << operandTwoEffect << "\n";

        // invariant
        if(operandOneEffect == INVARIANT && operandTwoEffect == INVARIANT){
                return true;
        }

        // increasing
        if(operandOneEffect == INCREASING && operandTwoEffect == INVARIANT){
                return true;
        }
        if(operandTwoEffect == INCREASING && operandOneEffect == INVARIANT){
                return true;
        }

        // decreasing
        if(operandOneEffect == DECREASING && operandTwoEffect == INVARIANT){
                return true;
        }
        if(operandTwoEffect == DECREASING && operandOneEffect == INVARIANT){
                return true;
        }

        // TODO monotonic increase and decrease

        return false;
}



bool LoopCheckPropagationPass::isInvariant(Value *operand)
{
        return false;
}



// returns INVARIANT, INCREASING, DECREASING, or WILD
effect_t LoopCheckPropagationPass::getEffect(Loop *loop, Value *operand)
{
        Constant *constant;
        if((constant = dyn_cast<Constant>(operand))){
                return INVARIANT;
        }

        // loop over all of the basic blocks in the loop.
        // check if there is an effect for that operand
        // if unchanged, do nothing
        // if changed, return WILD
        // if increment
        //      if >=0, inc                     |
        //      else wild                    <--| opportunities for more optimiziation here
        // if decrement                      <--| by tracking more accurately the changes
        //      if <=0, dec                     |
        //      else wild
        // if increment and -1, return wild
        // if decrement and 1, return wild
        // at the end, return increment for 1, decrement for -1, invariant for 0
        
        int change = 0;
        LoopBlocks *blocks = &loop->getBlocksVector();
        for(LoopBlocks::iterator it = blocks->begin(), ie = blocks->end(); it != ie; ++it){
                BasicBlock *block = *it;
                for(BasicBlock::iterator bbIt = block->begin(), bbIe = block->end(); bbIt != bbIe; ++bbIt){
                        Value *v = &*bbIt;
                        if(Instruction *inst = dyn_cast<Instruction>(v)){
                                MDNode *meta = inst->getMetadata("EFFECT");
                                if(meta != NULL){
                                        Value *affectedOperand = getAffectedOperandOfMeta(meta);

                                        if(operand == affectedOperand){
                                                std::string effect = getEffectOfMeta(meta);
                                                if(effect == "UNCHANGED"){
                                                        // do nothing
                                                }else if(effect == "INCREMENT"){
                                                       if(change >= 0){
                                                               ++change;
                                                       }else{
                                                               return WILD;
                                                       }
                                                }else if(effect == "DECREMENT"){
                                                        if(change <= 0){
                                                                --change;
                                                        }else{
                                                                return WILD;
                                                        }
                                                }else if(effect == "CHANGED"){
                                                        return WILD;
                                                }

                                        }

                                }
                        }
                }
        }

        if(change > 0){
                return INCREASING;
        }else if(change < 0){
                return DECREASING;
        }
        return INVARIANT;
}





std::string LoopCheckPropagationPass::getEffectOfMeta(MDNode *meta)
{
        return meta->getOperand(0)->getName().str();
}

Value *LoopCheckPropagationPass::getAffectedOperandOfMeta(MDNode *meta)
{
        return meta->getOperand(1);
}
