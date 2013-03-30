//===- RunTimeBoundsChecking.cpp - Instrumentation for run-time bounds checking --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a pass that instruments the code to perform run-time
// bounds checking on loads, stores, and other memory intrinsics.
//
//===----------------------------------------------------------------------===//


#define DEBUG_TYPE "run-time-bounds-checking"
#include "RunTimeBoundsChecking.h"
#include <set>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
using namespace llvm;

static cl::opt<bool> SingleTrapBB("bounds-checking-single-trap",
		cl::desc("Use one trap block per function"));

STATISTIC(ChecksAdded, "Bounds checks added");
STATISTIC(ChecksSkipped, "Bounds checks skipped");
STATISTIC(ChecksUnable, "Bounds checks unable to add");

char RunTimeBoundsChecking::ID = 0;
static RegisterPass<RunTimeBoundsChecking> X("rt-bounds-checking", "Run-time bounds checking", false, false);


/// getTrapBB - create a basic block that traps. All overflowing conditions
/// branch to this block. There's only one trap block per function.
BasicBlock *RunTimeBoundsChecking::getTrapBB() {
	if (TrapBB && SingleTrapBB)
		return TrapBB;

	Function *Fn = Inst->getParent()->getParent();
	BasicBlock::iterator PrevInsertPoint = Builder->GetInsertPoint();
	TrapBB = BasicBlock::Create(Fn->getContext(), "trap", Fn);
	Builder->SetInsertPoint(TrapBB);

	llvm::Value *F = Intrinsic::getDeclaration(Fn->getParent(), Intrinsic::trap);
	CallInst *TrapCall = Builder->CreateCall(F);
	TrapCall->setDoesNotReturn();
	TrapCall->setDoesNotThrow();
	TrapCall->setDebugLoc(Inst->getDebugLoc());
	Builder->CreateUnreachable();

	Builder->SetInsertPoint(PrevInsertPoint);
	return TrapBB;
}


/// emitBranchToTrap - emit a branch instruction to a trap block.
/// If Cmp is non-null, perform a jump only if its value evaluates to true.
void RunTimeBoundsChecking::emitBranchToTrap(Value* Offset, Value* Size, Value *Cmp) {
	// check if the comparison is always false
	ConstantInt *C = dyn_cast_or_null<ConstantInt>(Cmp);
	if (C) {
		errs() << "check skipped\n";
		++ChecksSkipped;
		if (!C->getZExtValue())
			return;
		else
			Cmp = 0; // unconditional branch
	}
	errs() << "check added\n";
	errs() << *Offset << "\n";
	errs() << *Size << "\n";
	//Instruction *Inst = Builder->GetInsertPoint();
	this->insertCheckBeforeInstruction(Offset, Size, Inst);
	BasicBlock *OldBB = Inst->getParent();
	BasicBlock *Cont = OldBB->splitBasicBlock(Inst);
	OldBB->getTerminator()->eraseFromParent();

	if (Cmp)
		BranchInst::Create(getTrapBB(), Cont, Cmp, OldBB);
	else
		BranchInst::Create(getTrapBB(), OldBB);
}


/// instrument - adds run-time bounds checks to memory accessing instructions.
/// Ptr is the pointer that will be read/written, and InstVal is either the
/// result from the load or the value being stored. It is used to determine the
/// size of memory block that is touched.
/// Returns true if any change was made to the IR, false otherwise.
bool RunTimeBoundsChecking::instrument(Value *Ptr, Value *InstVal) {
	uint64_t NeededSize = TD->getTypeStoreSize(InstVal->getType());
	DEBUG(dbgs() << "Instrument " << *Ptr << " for " << Twine(NeededSize)
			<< " bytes\n");

	SizeOffsetEvalType SizeOffset = ObjSizeEval->compute(Ptr);
	errs() << "type: " << InstVal->getName() << "\n";
	errs() << "Ptr: " << *Ptr << "\n";
	errs() << "instVal: " << *InstVal << "\n";
	errs() << "both known: " << ObjSizeEval->bothKnown(SizeOffset) << "\n";
	if (!ObjSizeEval->bothKnown(SizeOffset)) {
		++ChecksUnable;
		return false;
	}
	errs() << "both known: true\n";
	Value *Size   = SizeOffset.first;
	Value *Offset = SizeOffset.second;
	errs() << "size: " << *Size << "\n";
	errs() << "offset: " << *Offset << "\n";
	ConstantInt *SizeCI = dyn_cast<ConstantInt>(Size);

	Type *IntTy = TD->getIntPtrType(Ptr->getType());
	Value *NeededSizeVal = ConstantInt::get(IntTy, NeededSize);

	// three checks are required to ensure safety:
	// . Offset >= 0  (since the offset is given from the base ptr)
	// . Size >= Offset  (unsigned)
	// . Size - Offset >= NeededSize  (unsigned)
	//
	// optimization: if Size >= 0 (signed), skip 1st check
	// FIXME: add NSW/NUW here?  -- we dont care if the subtraction overflows
	Value *ObjSize = Builder->CreateSub(Size, Offset);
	Value *Cmp2 = Builder->CreateICmpULT(Size, Offset);
	Value *Cmp3 = Builder->CreateICmpULT(ObjSize, NeededSizeVal);
	Value *Or = Builder->CreateOr(Cmp2, Cmp3);
	if (!SizeCI || SizeCI->getValue().slt(0)) {
		///errs() << "sizeCI " << *SizeCI << "\n";
		Value *Cmp1 = Builder->CreateICmpSLT(Offset, ConstantInt::get(IntTy, 0));
		Or = Builder->CreateOr(Cmp1, Or);
	}
	emitBranchToTrap(Offset, Size, Or);
	++ChecksAdded;
	return true;
}

Constant* RunTimeBoundsChecking::createGlobalString(const StringRef& str)
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
	return globalStr;
}

bool RunTimeBoundsChecking::insertCheckBeforeInstruction(Value* offset, Value* size, Instruction* I)
{
	///const StringRef& variableName = I->getName();
	std::stringstream ss;
	ss << this->checkNumber;
	this->checkNumber++;
	Constant* globalStr = this->createGlobalString(ss.str());

	///errs() << "function " << *this->checkFunction << "\n";
	///errs() << "variable " << *globalStr << "\n";
	
	///create types of the arguments
	LLVMContext& context = this->M->getContext();
	Constant* gepFirstChar = ConstantExpr::getGetElementPtr(globalStr, this->gepFirstCharIndices);
	ConstantInt* lowerBound = ConstantInt::get(Type::getInt32Ty(context), 0);
	//ConstantInt* index = ConstantInt::get(Type::getInt32Ty(context), 1);
	//ConstantInt* upperBound = ConstantInt::get(Type::getInt32Ty(context), 2);
	///create vector with values which containts arguments values
	std::vector<Value*> argValuesV;
	argValuesV.push_back(gepFirstChar);
	argValuesV.push_back(lowerBound);
	argValuesV.push_back(offset);
	argValuesV.push_back(size);
	///create array ref to vector or arguments
	ArrayRef<Value*> argValuesA(argValuesV);
	///create call in code
	Builder->CreateCall4(this->checkFunction, gepFirstChar, lowerBound, offset, size);
	//CallInst* allocaCall = CallInst::Create(this->checkFunction, argValuesA, "", I);
	//allocaCall->setCallingConv(CallingConv::C);
	//allocaCall->setTailCall(false);
	return true;
}

bool RunTimeBoundsChecking::doInitialization(Module& M)
{
	std::vector<Type*> argTypes;
	this->checkNumber = 0;
	this->M = &M;

	///initialize some variables	
	///sets up the indices of the gep instruction for the first element	
	ConstantInt* zeroInt = ConstantInt::get(Type::getInt32Ty(this->M->getContext()), 0);
	this->gepFirstCharIndices.push_back(zeroInt);
	this->gepFirstCharIndices.push_back(zeroInt);
	///initialize all calls to library functions
	
	///scope start function
		
	///declared types
	Type* voidTy = Type::getVoidTy(M.getContext());
	Type* intTy = Type::getInt32Ty(M.getContext());
	Type* charPtrTy = Type::getInt8PtrTy(M.getContext());

	argTypes.push_back(charPtrTy);
	argTypes.push_back(intTy);
	argTypes.push_back(intTy);
	argTypes.push_back(intTy);

	///declared functions type calls
	ArrayRef<Type*> argArray(argTypes);
	FunctionType* checkFunctionType = FunctionType::get(voidTy, argArray, false);

	///create functions
	/*Constant* checkFunctionConstant =*/ M.getOrInsertFunction("check", checkFunctionType);
	
	///store Value to function
	this->checkFunction = M.getFunction("check");
	return true;
}

bool RunTimeBoundsChecking::runOnFunction(Function &F) {
	TD = &getAnalysis<DataLayout>();
	TLI = &getAnalysis<TargetLibraryInfo>();

	TrapBB = 0;
	BuilderTy TheBuilder(F.getContext(), TargetFolder(TD));
	Builder = &TheBuilder;
	ObjectSizeOffsetEvaluator TheObjSizeEval(TD, TLI, F.getContext());
	ObjSizeEval = &TheObjSizeEval;
	this->currentFunction = &F;

	// check HANDLE_MEMORY_INST in include/llvm/Instruction.def for memory
	// touching instructions
	std::vector<Instruction*> WorkList;
	for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
		Instruction *I = &*i;
		if (isa<LoadInst>(I) || isa<StoreInst>(I) || isa<AtomicCmpXchgInst>(I) ||
				isa<AtomicRMWInst>(I))
			WorkList.push_back(I);
	}

	bool MadeChange = false;
	for (std::vector<Instruction*>::iterator i = WorkList.begin(),
			e = WorkList.end(); i != e; ++i) {
		Inst = *i;
		errs() << "-----instruction inspected-----\n";
		errs() << "Inst: " << *Inst << "\n";
		Builder->SetInsertPoint(Inst);
		if (LoadInst *LI = dyn_cast<LoadInst>(Inst)) {
			MadeChange |= instrument(LI->getPointerOperand(), LI);
		} else if (StoreInst *SI = dyn_cast<StoreInst>(Inst)) {
			MadeChange |= instrument(SI->getPointerOperand(), SI->getValueOperand());
		} else if (AtomicCmpXchgInst *AI = dyn_cast<AtomicCmpXchgInst>(Inst)) {
			MadeChange |= instrument(AI->getPointerOperand(),AI->getCompareOperand());
		} else if (AtomicRMWInst *AI = dyn_cast<AtomicRMWInst>(Inst)) {
			MadeChange |= instrument(AI->getPointerOperand(), AI->getValOperand());
		} else {
			llvm_unreachable("unknown Instruction type");
		}
	}
	return MadeChange;
}
