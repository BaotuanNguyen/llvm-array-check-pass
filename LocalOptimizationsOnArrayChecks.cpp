#include "LocalOptimizationsOnArrayChecks.h"
#include "ArrayBoundsCheckPass.h"
#include "RunTimeBoundsChecking.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "stdlib.h"
#include <set>
#include <queue>
#include <vector>
#include <string>
#include <sstream>





using namespace llvm;

char LocalOptimizationsOnArrayChecks::ID = 0;
static RegisterPass<LocalOptimizationsOnArrayChecks> Y("local-opts", "Local optimizations on array checks performed", false, false);



bool LocalOptimizationsOnArrayChecks::doInitialization(Function& F)
{
        return false;
}



/*
 * create a data structure to recognize the call instructions
 */
bool LocalOptimizationsOnArrayChecks::runOnBasicBlock(BasicBlock& BB)
{
        
        for (BasicBlock::iterator i = BB.begin(), e = BB.end(); i != e; ++i) {
                Instruction *v = &*i;
                errs() << *i << "\n";
        }

        errs() << "iterating\n";
        //BB->dump();
        return false;
}
