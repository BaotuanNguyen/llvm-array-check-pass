
#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

using namespace llvm;

namespace {
	struct HelloPass : public FunctionPass
	{
		static char ID;
		HelloPass() : FunctionPass(ID) {}
		virtual bool runOnFunction(Function &F)
		{
			errs() << F.getName() << "\n";
			for(inst_iterator I = inst_begin(F), E = inst_end(F); I != E; I++)
			{
				if(isa<GetElementPtrInst>(*I))
				{
					errs() << "this is element ptr instruction\n";
				}
				errs() << *I << "\n";
			}
			
			return false;
		}
	};
}

char HelloPass::ID = 0;
static RegisterPass<HelloPass> X("hello", "hello pass", false, false);
