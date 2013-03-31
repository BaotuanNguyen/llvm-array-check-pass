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



bool LocalOptimizationsOnArrayChecks::runOnBasicBlock(BasicBlock& BB)
{

        return false;
}
