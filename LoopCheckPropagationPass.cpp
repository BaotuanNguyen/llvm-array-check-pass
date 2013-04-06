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
        //FIXME
        //DominatorTree dt = getAnalysis<DominatorTree>();
        errs() << "\n";
        errs() << "\n";
        errs() << "++++++++++++++++++++++++++++++++++++++++++++\n";
        errs() << "Beginning propagation of checks out of loops\n";
        errs() << "++++++++++++++++++++++++++++++++++++++++++++\n";
        errs() << "\n";
        return true;
}

bool LoopCheckPropagationPass::runOnLoop(Loop *L, LPPassManager &LPM)
{
        BasicBlock *bb = L->getUniqueExitBlock();
        errs() << *bb;
        errs() << *L;
        errs() << L->getLoopDepth() << "\n";
        LoopBlocks blocks = L->getBlocksVector();
        findCandidates(L, &blocks);
        return true;
}

bool LoopCheckPropagationPass::doFinalization(void)
{
	return true;
}

void LoopCheckPropagationPass::findCandidates(Loop *loop, LoopBlocks *blocks)
{
        hoist();
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
                                        
                                        if(loop->isLoopInvariant(operandOne)){
                                                errs() << "FOUND LOOP INVARIANT: " << *operandOne << "\n";
                                        }
                                        if(loop->isLoopInvariant(operandTwo)){
                                                errs() << "FOUND LOOP INVARIANT: " << *operandTwo << "\n";
                                        }
                                }
                        }
                }
        }
}

void LoopCheckPropagationPass::hoist(void)
{
        // ND is the set of all blocks that do not dominate all loop exits
        // first, get all loop exit blocks
        // for every block, check if it dominates all those loop exit blocks
        // if it does not, add it to ND
        // TODO 
        //DominatorTree *dt = new DominatorTree();
        //BasicBlock *bb = dt->getRoot();
        //errs() << "bb not knowing shit: " << *bb;

}
