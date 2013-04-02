#include "LocalOptimizationsOnArrayChecks.h"
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

char LocalOptimizationsOnArrayChecks::ID = 0;
static RegisterPass<LocalOptimizationsOnArrayChecks> C("local-opts", "Local optimizations on array checks performed", false, false);


bool LocalOptimizationsOnArrayChecks::doInitialization(Module& M)
{
        // stub function. do not delete. keeps the compiler warnings and errors at bay
        return false;
}

bool LocalOptimizationsOnArrayChecks::doInitialization(Function& F)
{
        errs() << "\n#########################################\n";
        errs() << "Beginning LocalOptimizationsOnArrayChecks\n";
        errs() << "#########################################\n";
        errs() << "#########################################\n\n";
        return false;
}




bool LocalOptimizationsOnArrayChecks::runOnBasicBlock(BasicBlock& BB)
{
        std::tr1::unordered_map<Instruction *, int> valueTable;
        int value = 0;

        errs() << "Inspecting a new basic block...\n";
        // hold an array of all of the call instructions in that basic block
        // every time we see a call instruction, loop over all the ones seen so far
        // if the exact check has already been made,
        // remove it

        /*
         * identical checks optimization:
         *  if C comes before C', and they are identical checks,
         *  then as long as the variables used in the check are not redefined between them,
         *  then we can eliminate C'
         */
        // iterate over all instructions in a basic block
        for (BasicBlock::iterator i = BB.begin(), e = BB.end(); i != e; ++i) {
                Instruction *inst = &*i;

                // insert every instruction into the value table.
                // those that have been seen already will have identical values
                errs() << "size before: " << valueTable.size() << "\n";
                vtInsert(valueTable, inst, value);
                errs() << "size after: " << valueTable.size() << "\n";

                errs() << "Instruction: " <<  *i << "\n";
                errs() << "Instruction getname:: " <<  i->getName() << "\n";

                // if it is a CallInst, then we may have a redundant check
                if (CallInst *ci = dyn_cast<CallInst> (inst)) {
                        errs() << "is call inst: " << *ci << "\n";

                        // get the called fn
                        Function *calledFn = ci->getCalledFunction();
                        errs() << "called fn: " <<  *calledFn << "\n";

                        // iterate over the operands
                        int numArgs = ci->getNumArgOperands();
                        for (int a = 0; a < numArgs; ++a) {
                                Value *v = ci->getArgOperand(a);
                                errs() << "arg " << a << ": " << *v << "\n";


                                // if an operand is also an instruction, we need to find where it was derived
                                if (Instruction *operandAsInstruction = dyn_cast<Instruction> (v)) {

                                        errs() << "cast on arg " << a << " worked\n";

                                        // for a simple identity check, it is sufficient to check
                                        // the target of the load
                                        Value *zeroOperand = operandAsInstruction->getOperand(0);
                                        if (LoadInst *loadInst = dyn_cast<LoadInst>(zeroOperand)) {
                                                errs() << "please be n: " <<  loadInst->getOperand(0)->getName() << "\n";
                                        }else{
                                                errs() << "sign extend instead: " << *zeroOperand << "\n";
                                        }

                                        User::const_op_iterator operandIterator = operandAsInstruction->op_begin();
                                        User::const_op_iterator end = operandAsInstruction->op_end();
                                        
                                        for (; operandIterator != end; operandIterator++) {
                                                errs() << "GOING GOING GOING: " << **operandIterator << "\n";

                                        }
                                }


                        }
                }
        }

        errs() << "Exiting basic block\n\n";
        return false;
}




void LocalOptimizationsOnArrayChecks::vtInsert(std::tr1::unordered_map<Instruction *, int> &table, Instruction *key, int value)
{
        errs() << "inside vtInsert with instruction: " << *key << "\n";

        // check if 
        std::tr1::unordered_map<Instruction *, int>::const_iterator el = table.find(key);
        
        if (el != table.end()) {
                errs() << "Not end\n";
        }else{
                errs() << "END\n";
        }

        std::pair<Instruction *, int> row(key, value);
        table.insert(row);


}


/*void LocalOptimizationsOnArrayChecks::isRedundantCheck(std::tr1::unordered_map<Instruction *, int> &table, Instruction *key, int value)
{



}*/











