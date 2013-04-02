#include "GlobalOptimizationsOnArrayChecks.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "stdlib.h"
#include <set>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include "ArrayBoundsCheckPass.h"





using namespace llvm;

char GlobalOptimizationsOnArrayChecks::ID = 0;
static RegisterPass<GlobalOptimizationsOnArrayChecks> D("global-opts", "Global optimizations on array checks performed", false, false);


bool GlobalOptimizationsOnArrayChecks::doInitialization(Module& M)
{
        // stub function. do not delete. keeps the compiler warnings and errors at bay
        return false;
}

bool GlobalOptimizationsOnArrayChecks::runOnFunction(Function& F)
{

        return true;
}


