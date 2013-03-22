
#include "ArrayBoundsCheckPass.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include <set>
#include <queue>

using namespace llvm;

char ArrayBoundsCheckPass::ID = 0;
char FunctionGetterModulePass::ID = 0;

static RegisterPass<FunctionGetterModulePass> X("fgmpass", "get function getter declaration pass", false, false);
static RegisterPass<ArrayBoundsCheckPass> Y("array-check", "Array Access Checks Inserted", false, false);

bool ArrayBoundsCheckPass::doInitialization(Module& M)
{
	this->M = &M;
	///initialize all calls to library functions
	
	///scope start function
	
	///declared types
	Type* voidTy = Type::getVoidTy(M.getContext());
	Type* intTy = Type::getInt32Ty(M.getContext());
	Type* charPtrTy = Type::getInt8PtrTy(M.getContext());
		

	///declared functions type calls
	FunctionType* allocaFunctionType = FunctionType::get(voidTy, charPtrTy, false);
	FunctionType* accessFunctionType = FunctionType::get(voidTy, intTy, false);
	FunctionType* scopeStartFunctionType = FunctionType::get(voidTy, false);
	FunctionType* scopeEndFunctionType = FunctionType::get(voidTy, false);


	///create functions
	Constant* scopeStartFunctionConstant = M.getOrInsertFunction("scopeStart", scopeStartFunctionType);
	Constant* accessFunctionConstant = M.getOrInsertFunction("arrayAccess", accessFunctionType);
	Constant* allocaFunctionConstant = M.getOrInsertFunction("alloca", allocaFunctionType);
	Constant* scopeEndFunctionConstant = M.getOrInsertFunction("scopeEnd", scopeEndFunctionType);
	
	errs() << "created function declaration " << *scopeStartFunctionConstant ;
	errs() << "created function declaration " << *accessFunctionConstant ;
	errs() << "created function declaration " << *allocaFunctionConstant ;
	errs() << "created function declaration " << *scopeEndFunctionConstant ;

	///store Value to function
	this->allocaFunction = M.getFunction("alloca");
	this->arrayAccessFunction = M.getFunction("arrayAccess");
	return true;
}

bool ArrayBoundsCheckPass::runOnFunction(Function& F)
{
	this->currentFunction = &F;
	this->findArrayAccess(F);	
	this->numBlockVisited++;
	return true;
}

Value* ArrayBoundsCheckPass::createGlobalString(const StringRef& str)
{
	///create string variable name
	std::string globalVarName = (std::string)this->currentFunction->getName();
	globalVarName.append(".");
	globalVarName.append((std::string)str);	

	///get the size of the variable name
	int arraySize = globalVarName.size()+1;

	///create an char array type
	Type* charTy = Type::getInt8Ty(this->M->getContext());
	ArrayType* strArrayTy = ArrayType::get(charTy, arraySize);

	///create the global variable
	GlobalVariable* globalStr = new GlobalVariable(*(this->M), strArrayTy, true, GlobalVariable::ExternalLinkage, 0, globalVarName, 0, GlobalVariable::NotThreadLocal, 0);
	globalStr->setAlignment(1);

	Constant* constStr = ConstantDataArray::getString(this->M->getContext(), globalVarName, true);
	globalStr->setInitializer(constStr);
	errs() << "creating global variable: " << *globalStr << "\n";
	return NULL;
}

bool ArrayBoundsCheckPass::findArrayAccess(Function& F)
{
	//iterator over instructions
	errs() << this <<" block " << this->numBlockVisited << " \n";	
	for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I)
	{
		Instruction* inst = &*I;
		if(GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(&*I))
		{
			errs() << *GEP << "[";
			if(Value* origin = this->findOriginOfPointer(GEP->getPointerOperand()))
			{
				errs() << *origin;
			}
			errs() << "]\n";
		}
		else if(AllocaInst* AI = dyn_cast<AllocaInst>(&*I))
		{
			this->collectVariableBeforeAlloca(AI);
		}
	}
	return true;
}

/*
 * Given a pointer in a GEP instruction, this will try to determine which
 * value the pointer came from
 */
Value* ArrayBoundsCheckPass::findOriginOfPointer(Value* pointer)
{
	
	std::set<Value*> originSet;
	std::set<Value*> valuesExploredSet;
	std::queue<Value*> valuesToExplore;

	valuesToExplore.push(pointer);

	while(!valuesToExplore.empty())
	{
		Value* currentValue = valuesToExplore.front();
		valuesToExplore.pop();

		//if this value has been explored already, then don't explore it
		if(valuesExploredSet.count(currentValue) > 0)
		{
			continue;
		}

		valuesExploredSet.insert(currentValue);

		if(CastInst* CAST = dyn_cast<CastInst>(currentValue))
		{
			//get the operand being cast, this will get us to memory location
			valuesToExplore.push(CAST->getOperand(0));
		}
		else if(PHINode* PHI = dyn_cast<PHINode>(currentValue))
		{
			unsigned int i;
			for(i = 0; i < PHI->getNumIncomingValues(); i++)
			{
				valuesToExplore.push(PHI->getIncomingValue(i));
			}
		}
		else if(GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(currentValue))
		{
			valuesToExplore.push(GEP->getPointerOperand());
		}
		else
		{
			//since we have a value and we don't know what to do with it, we should
			//add it origin set, if it is the only value in the origin set then this will
			//be the origin value.
			originSet.insert(currentValue);
		}
	}
	if(originSet.size() == 1)
	{
		return *(originSet.begin());
	}
	else
	{
		return NULL;
	}
}
