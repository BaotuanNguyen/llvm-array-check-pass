
#ifndef __ARRAY_BOUNDS_CHECK_PASS_H__
#define __ARRAY_BOUNDS_CHECK_PASS_H__
#include "llvm/User.h"
#include "llvm/BasicBlock.h"
#include "llvm/Pass.h"
#include "llvm/Operator.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Function.h"
#include "llvm/Instructions.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/InstrTypes.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/DataLayout.h"
#include "llvm/Target/TargetLibraryInfo.h"
//#include "RunTimeBoundsChecking.h"

#define LOWER 1 // for checking that an index is greater >- 0
#define UPPER 2 // for checking that an index is < limit

namespace llvm
{

	struct ArrayBoundsCheckPass : public FunctionPass
	{
		public:
			static char ID;
			ArrayBoundsCheckPass() : FunctionPass(ID) {}
			virtual bool doInitialization(Module& M);
			virtual bool runOnFunction(Function& F);
			Value* findOriginOfPointer(Value* pointer);
			virtual void getAnalysisUsage(AnalysisUsage& AU) const
			{
				AU.addRequired<DataLayout>();
				AU.addRequired<TargetLibraryInfo>();
                                //AU.addRequired<RunTimeBoundsChecking>();
			}
		private:
			/*check insertion functions*/
			Constant* createGlobalString(const StringRef& str);
			void die();
			//void checkGTZero(StringRef* varName, Value* index);
			//void checkLTLimit(StringRef* varName, Value* index, Value* limit);
			void checkGTZero(Value* basePointer, Value* index);
			void checkLTLimit(Value* basePointer, Value* index, Value* limit);
			//void insertCheck(StringRef* varName, int checkType, Value* index, Value* limit);
			void insertCheck(Value* basePointer, int checkType, Value* index, Value* limit);
			/*gep checker functions*/
			bool checkGEP(User* GEP, Instruction* currInst);
			bool runOnInstruction(Instruction* inst);
			bool runOnConstantExpression(ConstantExpr* CE, Instruction* currInst);

			unsigned int checkNumber;
			Module* M;		
			/*current function being checked*/
			Function* currentFunction;
			/*check function declared for check insertion*/
			Function* checkFunction;
			/*instruction before which to insert instructions*/
			Instruction* Inst;
			std::vector<Constant*> gepFirstCharIndices;
	};

}

#endif
