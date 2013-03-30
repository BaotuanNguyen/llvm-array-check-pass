#include "ArrayBoundsCheckPass.h"
#include "RunTimeBoundsChecking.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"
#include "stdlib.h"
#include <set>
#include <queue>
#include <vector>
#include <string>
#include <sstream>

using namespace llvm;

char ArrayBoundsCheckPass::ID = 0;
static RegisterPass<ArrayBoundsCheckPass> Y("array-check", "Array Access Checks Inserted", false, false);

Constant* ArrayBoundsCheckPass::createGlobalString(const StringRef& str)
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

void ArrayBoundsCheckPass::checkGTZero(StringRef* varName, Value* index)
{
	LLVMContext& context = this->M->getContext();
	ConstantInt* placeHolder = ConstantInt::get(Type::getInt32Ty(context), -1);
	this->insertCheck(varName, 1, index, placeHolder);
}

void ArrayBoundsCheckPass::checkLTLimit(StringRef* varName, Value* index, Value* limit)
{
	this->insertCheck(varName, 2, index, limit);	
}

void ArrayBoundsCheckPass::insertCheck(StringRef* varName, int checkType, Value* index, Value* limit)
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
	ConstantInt* checkTypeValue = ConstantInt::get(Type::getInt32Ty(context), checkType);
	//ConstantInt* index = ConstantInt::get(Type::getInt32Ty(context), 1);
	//ConstantInt* upperBound = ConstantInt::get(Type::getInt32Ty(context), 2);
	errs() << "Exectued this\n";
	CastInst* castInst = llvm::CastInst::CreateIntegerCast(index, Type::getInt32Ty(context), true, "", Inst);
	
	///create vector with values which containts arguments values
	std::vector<Value*> argValuesV;
	argValuesV.push_back(gepFirstChar);
	argValuesV.push_back(checkTypeValue);
	argValuesV.push_back(castInst);
	argValuesV.push_back(limit);
	///create array ref to vector or arguments
	errs() << "Exectued this\n";
	ArrayRef<Value*> argValuesA(argValuesV);
	///create call in code
	errs() << "Exectued this\n";
	CallInst* allocaCall = CallInst::Create(this->checkFunction, argValuesA, "", Inst);
	errs() << "Exectued this\n";
	allocaCall->setCallingConv(CallingConv::C);
	allocaCall->setTailCall(false);
}

bool ArrayBoundsCheckPass::doInitialization(Module& M)
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


bool ArrayBoundsCheckPass::runOnFunction(Function& F)
{
	this->currentFunction = &F;
	// iterate through instructions
	for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i)
	{
		Instruction* instr = &(*i);

		// examine each instruction's operands to find out any recursive GEP constant expressions
		// if an instruction itself is GEP, you need to check the array bounds of it
		if (GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(instr))
		{
			errs() << "----------------------------------------------------------------------\n";
			errs() << "[GEP instruction detected]: " << *GEP << "\n";
			checkGEP(GEP, GEP); // check array bounds for GEP Instruction as well as examine operands
		}
		else
		{
			runOnInstruction(&(*i)); // examine each instruction's operand
		}
	}
	return true;
}

bool ArrayBoundsCheckPass::runOnInstruction(Instruction* inst)
{
	// iterate through operands
	for (unsigned pos = 0; pos < inst->getNumOperands(); ++pos)
	{
		Value* operand = inst->getOperand(pos);

		// if an operand is a constant expression, further examine whether it is a GEP expression
		if (ConstantExpr* expr = dyn_cast<ConstantExpr>(operand))
		{
			runOnConstantExpression(expr, inst);
		}
	}

	return true;
}

bool ArrayBoundsCheckPass::runOnConstantExpression(ConstantExpr* CE, Instruction* currInst)
{
	// iterate recursively through operands to visit every single constant expression
	for (unsigned pos = 0; pos < CE->getNumOperands(); ++pos)
	{
		Value* operand = CE->getOperand(pos);

		if (ConstantExpr* expr = dyn_cast<ConstantExpr>(operand))
		{
			runOnConstantExpression(expr, currInst);
		}
	}

	// if this expression is a GEP expression, check the array bounds
	if (CE->getOpcode() == Instruction::GetElementPtr)
	{
		errs() << "----------------------------------------------------------------------\n";
		errs() << "[GEP expression detected]: " << *currInst << "\n";

		checkGEP(CE, currInst);
	}

	return true;
}


// check array bounds of a GEP instruction or a GEP expression
// I think it is sufficient to check whether FIRST index of GEP instruction is greater than 0 (except in VLA case)
bool ArrayBoundsCheckPass::checkGEP(User* user, Instruction* currInst)
{
	int position = 0; // operand position
	
	gep_type_iterator GEPI = gep_type_begin(user), E = gep_type_end(user);
	User::const_op_iterator OI = user->op_begin();
			
	Value* basePointer;

	// iterate through array types (which equal to the number of operands in GEP - 1)
	for (; GEPI != E; ++GEPI, ++OI, ++position)
	{
		// make sure you visit all constant expressions
		if (ConstantExpr *CE = dyn_cast<ConstantExpr>(*OI))
		{
			runOnConstantExpression(CE, currInst);
		}
		
		// first operand contains Base Pointer information
		if (position == 0)
		{
			errs() << "Base Pointer Type: " << **GEPI << "\n";
			errs() << "Base Pointer " << **OI << "\n";
			
			basePointer = *OI;	
			OI++; // now OI points to the first index position
			
			// second operand contains "first" index
			if (ConstantInt *CI = dyn_cast<ConstantInt>(*OI))
			{
				uint64_t firstIndex = CI->getZExtValue();
				errs() << "\nFirst Index (Constant): " << firstIndex << "\n";
			
				// if there is only one index in GEP and Base Pointer is alloca, then this must be a VLA
				if ((OI+1) == user->op_end())
				{
					if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(basePointer))
					{
						// Insert runtime check 
						errs() << "VLA Detected\n";
						this->Inst = currInst;
						StringRef* basePointerName = new StringRef((std::string)basePointer->getName());
						errs() << "1 executed this ," << *CI << ", " << *allocaInst->getOperand(0) <<"\n";
						this->checkLTLimit(basePointerName, CI, allocaInst->getOperand(0));
						this->checkGTZero(basePointerName, CI);
					}
					else
					{
						errs() << "This GEP is not an array access\n";
					}
				}		
				// Otherwise, if "first" index is greater than 0, then this array access is out of bound
				else if (CI->getZExtValue() > 0)
				{
					errs() << "GEP index " << firstIndex << " is out of bound at operand position " << position << "\n";
					exit(-1);
				}				
			}
			else // First index is in non-constant form
			{
				errs() << "\nFirst Index (Non-constant): " << **OI << "\n";
			
				if (Instruction* instr = dyn_cast<Instruction>(*OI))
				{
					errs() << instr->getName() << "\n";
				}
				// if there is only one index in GEP and Base Pointer is alloca, then this must be a VLA
				if ((OI+1) == user->op_end())
				{
					if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(basePointer))
					{
						// Insert runtime check 
						errs() << "VLA Detected\n";
						errs() << "Check: If First Index > BasePointer Allocation, call die();\n";
						this->Inst = currInst;
						StringRef* basePointerName = new StringRef((std::string)basePointer->getName());
						errs() << "2 executed this ," << *OI << ", " << *allocaInst->getOperand(0) <<"\n";
						this->checkLTLimit(basePointerName, *OI, allocaInst->getOperand(0));
						this->checkGTZero(basePointerName, *OI);
					}
					else
					{
						errs() << "This GEP is not an array access\n";
					}
				}					
				
				//else if
				//{
					//add dynamic check insertion here
				//}
			}
		}
		// bound checking for indices other than the "first" index
		else
		{
			if (ConstantInt *CI = dyn_cast<ConstantInt>(*OI)) // index in constant form
			{
				if (ArrayType *Aty = dyn_cast<ArrayType>(*GEPI))
				{
					uint64_t index = CI->getZExtValue();
					uint64_t numElements = Aty->getNumElements();

					errs() << "\nIndex (Constant): " << index << "\n";
					errs() << "numElements: " << numElements << "\n"; // number of elements in this part of array
				
					// index cannot go above number of elements - 1
					if (CI->getZExtValue() >= Aty->getNumElements())
					{
						errs() << "GEP index " << index << " is out of bound at operand position " << position << "\n";
						exit(1);
					}
				}
			}
			else // index is in non-constant form
			{
				if (ArrayType *Aty = dyn_cast<ArrayType>(*GEPI))
				{
					errs() << "\nIndex (Non-constant): " << **OI << "\n";
					errs() << "NumElements: " << Aty->getNumElements() << "\n";
					this->Inst = currInst;
					LLVMContext& context = this->M->getContext();
					ConstantInt* arraySizeCI = ConstantInt::get(Type::getInt32Ty(context), Aty->getNumElements());
					StringRef* basePointerName = new StringRef((std::string)basePointer->getName());
					errs() << "3 executed this ," << **OI << ", " << *arraySizeCI <<"\n";
					this->checkLTLimit(basePointerName, *OI, arraySizeCI);
					this->checkGTZero(basePointerName, *OI);
				}
			}
		}
	}

	errs() << "----------------------------------------------------------------------\n";

	return true;
}

