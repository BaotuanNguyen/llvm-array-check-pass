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
        this->bbAndInstVec = new BBAndInstVec();

        return true;
}

bool LoopCheckPropagationPass::runOnLoop(Loop *L, LPPassManager &LPM)
{
        errs() << "\n***\n";
        errs() << "Finding candidates\n";
        errs() << "***\n";
        findCandidates(L); // populates the this->bbToCheck (mapping of basic blocks to their candidate checks)

        errs() << "\n***\n";
        errs() << "Preparing for hoist\n";
        errs() << "***\n";
        prepHoist(L); // populates bbAndInstVec

        errs() << "\n***\n";
        errs() << "Hoisting, good sirs\n";
        errs() << "***\n";

        hoist();


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
				if(callFunctionName.equals("checkLessThan")){
                                        errs() << "callinst: " << *ci << "\n";
                                        MDNode* metadata = ci->getMetadata("VarName");
                                        errs() << "metadata: " << *metadata;

                                        Value *operandOne = metadata->getOperand(0);
                                        Value *operandTwo = metadata->getOperand(1);

                                        if(isCandidate(loop, operandOne, operandTwo)){

                                                errs() << "candidate operandOne: " << *operandOne << "\n";
                                                errs() << "candidate operandTwo: " << *operandTwo << "\n";
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


void LoopCheckPropagationPass::prepHoist(Loop *loop)
{


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

        // gather all candidate checks and th
        // remove them from their current blocks
        if(validPredecessorBlocks->size() > 0){
                // iterate over candidate blocks
                for(BBToCheckSet::iterator it = bbToCheck->begin(), ie = bbToCheck->end(); it != ie; ++it){
                        // iterate over the candidate checks within a given block
                        CheckSet *checkSet =  it->second;
                        for(CheckSet::iterator csIt = checkSet->begin(), csIe = checkSet->end(); csIt != csIe; ++csIt){
                                // remove the instruction from its current block
                                // add the instruction to header's predecessors -- we're just going to skip everything in the paper
                                for(PredBlocks::iterator predIt = validPredecessorBlocks->begin(), predIe = validPredecessorBlocks->end(); predIt != predIe; ++predIt){
                                        bbAndInstVec->push_back(new PairBBAndInst(*predIt, *csIt));
                                }
                        }
                }
        }


}


void LoopCheckPropagationPass::hoist(void)
{

        // remove and insert instructions
        for(BBAndInstVec::iterator it = bbAndInstVec->begin(), ie = bbAndInstVec->end(); it != ie; ++it){
                errs() << "remove it\n";

                /*
                 * the check call is dependent on several instructions.
                 * to simplify the algorithm, we make the assumption that those dependencies
                 * directly precede the instruction.
                 * because there is another check call that may rely on those same instructions, we
                 * can first check to see if those instructions have been moved already
                 */

                PairBBAndInst *bni = *it;
                BasicBlock *predBlock = bni->first; // the predecessor block into which we are hoisting
                Instruction *inst = bni->second; // the check instruction we are hoisting
                BasicBlock *instBlock = inst->getParent(); // the block of the check instruction
                Instruction *back = &predBlock->back(); // the last instruction in the block into which we are hoisting
                typedef std::vector<Instruction *> MoveVec;
                MoveVec *moveVec = new MoveVec(); // the vector of instructions being moved
                std::vector<Instruction *> *instDependenciesVec = new std::vector<Instruction *>();
                //inst->eraseFromParent(); // this, without the other two calls, works

                // these two calls together dont work because %idxprom is referenced by the call
                //inst->removeFromParent();
                //block->getInstList().insert(back, inst);

	        llvm::BasicBlock::InstListType& instList = instBlock->getInstList();
                bool keepPushing = false;
                for(BasicBlock::InstListType::reverse_iterator rStart = instList.rbegin(), rEnd = instList.rend(); rStart != rEnd; ++rStart){
                        Instruction *instTemp = &*rStart;
                        errs() << "instTemp: " << *instTemp << "\n";
                        if(keepPushing){
                                if(CallInst *ci = dyn_cast<CallInst> (instTemp)){
                                        const StringRef& callFunctionName = ci->getCalledFunction()->getName();
                                        if(callFunctionName.equals("checkLessThan")){
                                                continue;
                                        }else{
                                                // impossible
                                        }
                                }else{
                                        // instTemp is needed by the check call
                                        errs() << "pushing: " << *instTemp << "\n";
                                        // we want to push all instructions up to the load of the invariant
                                        moveVec->push_back(instTemp);
                                        if(LoadInst *li = dyn_cast<LoadInst>(instTemp)) {
                                                keepPushing = false;
                                        }
                                }
                        }
                        if(instTemp == inst){
                                errs() << "Yo wuddup yo\n";
                                moveVec->push_back(instTemp);
                                // find the values it needs
                                keepPushing = true;
                        }
                }

                // remove and insert
                while(moveVec->size() > 0){
                        Instruction *instToRemove = moveVec->back();
                        errs() << "instToRemove: " << *instToRemove << "\n";
                        moveVec->pop_back();

                        // DO NOT DELETE: example code for cloning
                        // this has problems because the names of the 
                        // variables in the instructions stay the same,
                        // so the definition of those variables may not come
                        // until later
                        /*Instruction *clonedInst = instToRemove->clone();
                        if(!clonedInst->getType()->isVoidTy()){
                                //clonedInst->setName("yo");
                                clonedInst->setName(instToRemove->getName());
                        }
                        errs() << clonedInst->getName().str() << "\n";
                        predBlock->getInstList().insert(back, clonedInst);*/

                        instToRemove->removeFromParent();
                        predBlock->getInstList().insert(back, instToRemove);
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
        // if decrement                      <--| by tracking the changes more accurately
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




/// old hoist code that may be helpful later
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

