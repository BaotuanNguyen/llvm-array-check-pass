#ifndef __EFFECT_GEN_H__
#define __EFFECT_GEN_H__

#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Operator.h"
#include "llvm/Support/GetElementPtrTypeIterator.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "ArrayBoundsCheckPass.h"
#include "RangeCheckSet.h"
#include <queue>
#include <set>
#include <map>

namespace llvm 
{
	class EffectGen 
	{
		public: 
			static void generateEffect(Instruction* inst, Module* M);
		private:
			static void generateMetadata(MDString* str, Value* variable, Value* n, Instruction* inst, Module* M);
	};
}

#endif
