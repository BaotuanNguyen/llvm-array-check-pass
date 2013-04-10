#ifndef __ARRAY_BOUNDS_CHECK_PASS_H__
#define __ARRAY_BOUNDS_CHECK_PASS_H__

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Operator.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Instructions.h"
#include <queue>
#include <set>

using namespace llvm;

namespace llvm
{
	struct ArrayBoundsCheckPass : public ModulePass
	{
		public:
			static char ID;
			ArrayBoundsCheckPass() : ModulePass(ID) {}
			virtual bool doInitialization(Module& M);
			virtual bool doFinalization(Module& M);
			virtual bool runOnModule(Module& F);
			Value* findOriginOfPointer(Value* pointer);
			virtual void getAnalysisUsage(AnalysisUsage& AU) const
			{
//				AU.addRequired<EffectGenPass>();
			}
			int checkNumber;
		
		private:
			/*check insertion functions*/
			//Constant* createGlobalString(const StringRef* str);
			Instruction* checkLessThan(Value* left, Value* right);
			Instruction* insertLessThanCheck(Value* left, Value* right);
			
			/*gep checker functions*/
			bool checkGEP(User* GEP, Instruction* currInst);
			bool runOnInstruction(Instruction* inst);
			bool runOnConstantExpression(ConstantExpr* CE, Instruction* currInst);
			bool runOnFunction(Function* F);

			Module* M;
			/*current function being checked*/
			Function* currentFunction;
			/*check function declared for check insertion*/
			Function* checkLessThanFunction;
			/*instruction before which to insert instructions*/
			Instruction* Inst;
	};

}
#endif
