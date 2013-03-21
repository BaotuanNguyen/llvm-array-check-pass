
#ifndef __ARRAY_BOUNDS_CHECK_PASS_H__
#define __ARRAY_BOUNDS_CHECK_PASS_H__
#include "llvm/Pass.h"
#include "llvm/DataLayout.h"
#include "llvm/Target/TargetLibraryInfo.h"

namespace llvm
{

struct ArrayBoundsCheckPass : public FunctionPass
{
	public:
		static char ID;
		ArrayBoundsCheckPass() : FunctionPass(ID) {
			this->numBlockVisited = 0;
		}
		virtual bool runOnFunction(Function& F);
		bool findArrayAccess(Function& F);
		Value* findOriginOfPointer(Value* pointer);
		virtual void getAnalysisUsage(AnalysisUsage& AU) const
		{
			AU.addRequired<DataLayout>();
			AU.addRequired<TargetLibraryInfo>();
		}
	private:
		unsigned int numBlockVisited;
};

}

#endif
