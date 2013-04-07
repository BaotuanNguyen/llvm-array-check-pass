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

        this->bbToCheck = new BBToCheckSet();

        return true;
}

bool LoopCheckPropagationPass::runOnLoop(Loop *L, LPPassManager &LPM)
{
        errs() << "\n***\n";
        errs() << "Finding candidates\n";
        errs() << "***\n";
        findCandidates(L); // populates the this->bbToCheck (mapping of basic blocks to their candidate checks)
        errs() << "\n***\n";
        errs() << "Hoisting, good sirs\n";
        errs() << "***\n";
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
                                                errs() << "so, i got this candidate...\n";

                                                // CallInst ci is a candidate check for BasicBlock block
                                                // look up the basic block
                                                // if it doesn't exist,
                                                //      create a new set
                                                // add the call instruction to the set
                                                // if it didnt exist,
                                                //      insert the mapping
                                                BBToCheckSet::iterator it = bbToCheck->find(block);
                                                CheckSet *cs;
                                                if(it != bbToCheck->end())
                                                {
                                                        // element found
                                                        // set the checkset var
                                                        cs = it->second;
                                                }else{
                                                        // element not found
                                                        // create the checkste and the mapping
                                                        cs = new CheckSet();
                                                        bbToCheck->insert(PairBBAndCheckSet(block, cs));
                                                }

                                                // insert the call instruction to the set of candidate checks
                                                cs->insert(ci);

                                                //bbToCheck->insert(PairBBAndCheck(block, ci));
                                                //errs() << "candidate ci " << *ci << "\n";
                                        }
                                }
                        }
                }
        }
}


void LoopCheckPropagationPass::hoist(Loop *loop)
{
        // DO NOT DELETE... may want this later if we follow the paper more closely
        /*LoopBlocks *blocks = &loop->getBlocksVector();
        for(LoopBlocks::iterator it = blocks->begin(), ie = blocks->end(); it != ie; ++it){
                BasicBlock *block = *it;

                //errs() << "hoisting\n";
                std::vector<BasicBlock *> ND; 
                // ND is the set of all blocks that do not dominate all loop exits
                // Cn is the set of candidate checks c, s.t. at the entry to n, we can assert that c will be executed in n
                //   -- captured by this->bbToCheck
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
                if(numDominated != loopExitingBlocks.size()){
                        // block does not dominate all exiting blocks
                        errs() << "ND pushing back\n";
                        ND.push_back(block);
                }
        }*/


        // adjust the paper's algorithm:
        //  grab the header of the loop.
        //  try hoisting all candidate checks into the header and go from there.

        // all predecessors going into the loop need to add the instruction...
        // but if the predecessor is in the loop, we don't include it
        typedef std::vector<BasicBlock *> PredBlocks;
        //std::vector<BasicBlock *> *validPredecessorBlocks = new std::vector<BasicBlock *>();
        PredBlocks *validPredecessorBlocks = new PredBlocks();

        BasicBlock *header = loop->getHeader();//->getUniquePredecessor();
        for(pred_iterator it = pred_begin(header), ie = pred_end(header); it != ie; ++it){
                LoopBlocks *loopBlocks = &loop->getBlocksVector();
                if(std::find(loopBlocks->begin(), loopBlocks->end(), *it) == loopBlocks->end()) {
                        // loopBlocks does not contain the predecessor
                        validPredecessorBlocks->push_back(*it);
                }
        }
        // insert candidate checks into predecessor blocks
        // remove them from their current blocks
        if(validPredecessorBlocks->size() > 0){
                // iterate over candidate blocks
                for(BBToCheckSet::iterator it = bbToCheck->begin(), ie = bbToCheck->end(); it != ie; ++it){
                        // iterate over the candidate checks within a given block
                        errs() << "yo\n"; 
                        CheckSet *checkSet =  it->second;
                        for(CheckSet::iterator csIt = checkSet->begin(), csIe = checkSet->end(); csIt != csIe; ++csIt){
                                // remove the instruction from its current block
                                // add the instruction to header's predecessors -- we're just going to skip everything in the paper
                                errs() << "BIG STINKING MAMAAAAAAAAAAAAAAAAAAAAAAAAA\n";
                                (*csIt)->eraseFromParent();
                                for(PredBlocks::iterator predIt = validPredecessorBlocks->begin(), predIe = validPredecessorBlocks->end(); predIt != predIe; ++predIt){
                                        (*predIt)->getInstList().push_back(*csIt);
                                }
                        }
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
