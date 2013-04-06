#include "ArrayBoundsCheckPass.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/InstrTypes.h"
#include "stdlib.h"
#include <set>
#include <queue>

using namespace llvm;

char ArrayBoundsCheckPass::ID = 0;
static RegisterPass<ArrayBoundsCheckPass> Y("array-check", "Insert Run-Time Array Bounds Checks", false, false);

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

		if (CastInst* CAST = dyn_cast<CastInst>(currentValue))
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
			////add it origin set, if it is the only value in the origin set then this will
			////be the origin value.
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

Instruction* ArrayBoundsCheckPass::checkGTLimit(Value* lb, Value* index)
{
	return this->insertGTLimitCheck(lb, index);
}

Instruction* ArrayBoundsCheckPass::checkLTLimit(Value* index, Value* ub)
{
	return this->insertLTLimitCheck(index, ub);	
}

Instruction* ArrayBoundsCheckPass::insertGTLimitCheck(Value* lv, Value* index)
{
	this->checkNumber++;
	LLVMContext& context = this->M->getContext();
	
	if (index->getType() == Type::getInt32Ty(context))
		index = llvm::CastInst::CreateIntegerCast(index, Type::getInt64Ty(context), true, "", Inst);
	
	std::vector<Value*> argValuesV;
	argValuesV.push_back(lv);
	argValuesV.push_back(index);
	
	///create array ref to vector or arguments
	ArrayRef<Value*> argValuesA(argValuesV);

	///create call in code
	CallInst* allocaCall = CallInst::Create(this->checkGTLimitFunction, argValuesA, "", Inst);
	
	allocaCall->setCallingConv(CallingConv::C);
	allocaCall->setTailCall(false);
	
	return allocaCall;
}

Instruction* ArrayBoundsCheckPass::insertLTLimitCheck(Value* index, Value* limit)
{
	this->checkNumber++;
	
	///create types of the arguments
	LLVMContext& context = this->M->getContext();
	
	if (index->getType() == Type::getInt32Ty(context))
		index = llvm::CastInst::CreateIntegerCast(index, Type::getInt64Ty(context), true, "", Inst);
	
	if (limit->getType() == Type::getInt32Ty(context))
		limit = llvm::CastInst::CreateIntegerCast(limit, Type::getInt64Ty(context), true, "", Inst);
	
	///create vector with values which containts arguments values
	std::vector<Value*> argValuesV;
	argValuesV.push_back(index);
	argValuesV.push_back(limit);
	
	///create array ref to vector or arguments
	ArrayRef<Value*> argValuesA(argValuesV);

	///create call in code
	CallInst* allocaCall = CallInst::Create(this->checkLTLimitFunction, argValuesA, "", Inst);
	
	allocaCall->setCallingConv(CallingConv::C);
	allocaCall->setTailCall(false);

	return allocaCall;
}

bool ArrayBoundsCheckPass::doInitialization(Module& M)
{
	std::vector<Type*> argTypes1;
	std::vector<Type*> argTypes2;

	this->checkNumber = 0;
	this->M = &M;

	Type* voidTy = Type::getVoidTy(M.getContext());
	Type* intTy = Type::getInt64Ty(M.getContext());
	//Type* charPtrTy = Type::getInt8PtrTy(M.getContext());

	argTypes1.push_back(intTy);
	argTypes1.push_back(intTy);
	
	argTypes2.push_back(intTy);
	argTypes2.push_back(intTy);


	//declared functions type calls
	ArrayRef<Type*> argArray1(argTypes1);
	ArrayRef<Type*> argArray2(argTypes2);
	
	FunctionType* checkGTLimitFunctionType = FunctionType::get(voidTy, argArray1, false);
	FunctionType* checkLTLimitFunctionType = FunctionType::get(voidTy, argArray2, false);

	//create functions
	M.getOrInsertFunction("checkGTLimit", checkGTLimitFunctionType);
	M.getOrInsertFunction("checkLTLimit", checkLTLimitFunctionType);
	
	//store Value to function
	this->checkGTLimitFunction = M.getFunction("checkGTLimit");
	this->checkLTLimitFunction = M.getFunction("checkLTLimit");
	return true;
}

bool ArrayBoundsCheckPass::doFinalization(Module& M)
{
	errs() << "-------------------------------------------------------\n";
	errs() << "Number of checks inserted: " <<  this->checkNumber << "\n";
	errs() << "-------------------------------------------------------\n";
	return true;
}

bool ArrayBoundsCheckPass::runOnModule(Module& M)
{
	doInitialization(M);
	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i)
	{
		Function* func = &(*i);
		runOnFunction(&(*func));
	}
	doFinalization(M);

	return true;
}

bool ArrayBoundsCheckPass::runOnFunction(Function* F)
{
	this->currentFunction = F;
	// iterate through instructions
	for (inst_iterator i = inst_begin(*F), e = inst_end(*F); i != e; ++i)
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
	Value* originPointer;

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
			basePointer = *OI;	
			originPointer = findOriginOfPointer(*OI);
			
			if (!originPointer)
			{
				errs() << "origin pointer returned NULL. SHOULD NOT HAPPEN!!!\n";
			}
			
			errs() << "Base Pointer Type: " << **GEPI << "\n";
			errs() << "Base Pointer " << *basePointer << "\n";
			errs() << "Origin Base Pointer: " << *originPointer << "\n";
		
			OI++; // now OI points to the first index position
			
			// second operand contains "first" index
			if (ConstantInt *CI = dyn_cast<ConstantInt>(*OI))
			{
				int64_t firstIndex = CI->getSExtValue();
				errs() << "\nFirst Index (Constant): " << firstIndex << "\n";
			
				// if there is only one index in GEP and Base Pointer is alloca, then this must be a VLA
				if ((OI+1) == user->op_end())
				{
					if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(basePointer))
					{
						errs() << "VLA Detected1\n";
						this->Inst = currInst;
						
						Value* limit = allocaInst->getOperand(0);
						Value* originLimit = findOriginOfPointer(limit);

						if (dyn_cast<LoadInst>(originLimit))
							originLimit = ((LoadInst*) originLimit)->getOperand(0);
				//		else
				//			errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";

						errs() << "index: " << *CI << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "var1: " << *CI << "\n";
						errs() << "var2: " << *originLimit << "\n";
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
						
						varNames1.push_back(CI);
						varNames1.push_back(originLimit);

						ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(context), 0);
						
						varNames2.push_back(zero);
						varNames2.push_back(CI);
						
						MDNode* meta_LT = MDNode::get(context, varNames1);
						MDNode* meta_GT = MDNode::get(context, varNames2);

						Instruction* callLTLimit = this->checkLTLimit(CI, limit);
						Instruction* callGTZero = this->checkGTLimit(zero, CI);
						
						callLTLimit->setMetadata("VarName", meta_LT);
						callGTZero->setMetadata("VarName", meta_GT);
					}
					else if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(originPointer))
					{
						// Insert runtime check 
						errs() << "VLA Chain Detected1\n";
						this->Inst = currInst;
						StringRef* indexName = new StringRef((basePointer->getName()).str() + ".idx");

						BinaryOperator* index = BinaryOperator::Create(Instruction::Add, dyn_cast<User>(basePointer)->getOperand(1), &(**OI), Twine(*indexName), currInst);
						Value* limit = allocaInst->getOperand(0);
						Value* originLimit = findOriginOfPointer(limit);

						if (dyn_cast<LoadInst>(originLimit))
							originLimit = ((LoadInst*) originLimit)->getOperand(0);
					//	else
					//		errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";
						
						//firstOperand of basePointer + index < limit;
						errs() << "Origin Pointer: " << *originPointer << "\n";
						
						errs() << "index: " << *index << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "var1: " << *index << "\n";
						errs() << "var2: " << *originLimit << "\n";
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;

						varNames1.push_back(index);
						varNames1.push_back(limit);
						
						ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(context), 0);
						
						varNames2.push_back(zero);
						varNames2.push_back(index);
						
						MDNode* meta_LT = MDNode::get(context, varNames1);
						MDNode* meta_GT = MDNode::get(context, varNames2);

						Instruction* callLTLimit = this->checkLTLimit(index, limit);
						Instruction* callGTZero = this->checkGTLimit(zero, index);
						
						callLTLimit->setMetadata("VarName", meta_LT);
						callGTZero->setMetadata("VarName", meta_GT);
					}
					else
					{
						errs() << "This GEP is not an array access\n";
					}
				}		
				// Otherwise, if "first" index is greater than 0, then this array access is out of bound
				else
				{
				   	if (CI->getZExtValue() > 0)
					{
							errs() << "First index " << firstIndex << " is greater than 0";
							errs() << "Compile-time analysis detected an out-of-bound access! Terminating...\n";
							exit(1);
					}
					else
					{
							errs() << "First index " << firstIndex << " passed compile-time check: 0 <= " << firstIndex << "\n";
					}
				}			
			}
			else // First index is in non-constant form
			{
				errs() << "\nFirst Index (Non-constant): " << **OI << "\n";
		
				// if there is only one index in GEP and Base Pointer is alloca, then this must be a VLA
				if ((OI+1) == user->op_end())
				{
					if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(basePointer))
					{
						// Insert runtime check 
						errs() << "VLA Detected2\n";
						this->Inst = currInst;

						Value* originIndex = findOriginOfPointer(*OI);
						Value* limit = allocaInst->getOperand(0);
						Value* originLimit = findOriginOfPointer(limit);

						if (dyn_cast<LoadInst>(originIndex))
							originIndex = ((LoadInst*) originIndex)->getOperand(0);
				//		else
				//			errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";

						if (dyn_cast<LoadInst>(originLimit))
							originLimit = ((LoadInst*)originLimit)->getOperand(0);
				//		else
				//			errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";
						
						errs() << "index: " << **OI << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "var1: " << *originIndex << "\n";
						errs() << "var2: " << *originLimit << "\n";
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
						
						varNames1.push_back(originIndex);
						varNames1.push_back(originLimit);
						
						ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(context), 0);
						
						varNames2.push_back(zero);
						varNames2.push_back(originIndex);
						
						MDNode* meta_LT = MDNode::get(context, varNames1);
						MDNode* meta_GT = MDNode::get(context, varNames2);

						Instruction* callLTLimit = this->checkLTLimit(*OI, limit);
						Instruction* callGTZero = this->checkGTLimit(zero, *OI);
						
						callLTLimit->setMetadata("VarName", meta_LT);
						callGTZero->setMetadata("VarName", meta_GT);
					}
					else if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(originPointer))
					{
						// Insert runtime check 
						errs() << "VLA Chain Detected2\n";
						this->Inst = currInst;
						StringRef* indexName = new StringRef((basePointer->getName()).str() + ".idx");
						
						BinaryOperator* index = BinaryOperator::Create(Instruction::Add, dyn_cast<User>(basePointer)->getOperand(1), &(**OI), Twine(*indexName), currInst);

						Value* originIndex = findOriginOfPointer(*OI);
						Value* limit = allocaInst->getOperand(0);
						Value* originLimit = findOriginOfPointer(limit);

						if (dyn_cast<LoadInst>(originIndex))
							originIndex = ((LoadInst*)originIndex)->getOperand(0);
				//		else
				//			errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";

						if (dyn_cast<LoadInst>(originLimit))
							originLimit = ((LoadInst*)originLimit)->getOperand(0);
				//		else
				//			errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";
					
						//firstOperand of basePointer + index < limit;
						errs() << "Origin Pointer: " << *originPointer << "\n";
						
						errs() << "index: " << *index << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "var1: " << *originIndex << "\n";
						errs() << "var2: " << *originLimit << "\n";
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
						
						varNames1.push_back(originIndex);
						varNames1.push_back(originLimit);
						
						ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(context), 0);
						
						varNames2.push_back(zero);
						varNames2.push_back(originIndex);
					
						MDNode* meta_LT = MDNode::get(context, varNames1);
						MDNode* meta_GT = MDNode::get(context, varNames2);

						Instruction* callLTLimit = this->checkLTLimit(*OI, limit);
						Instruction* callGTZero = this->checkGTLimit(zero, *OI);
						
						callLTLimit->setMetadata("VarName", meta_LT);
						callGTZero->setMetadata("VarName", meta_GT);
					}
					else
					{
						errs() << "This GEP is not an array access\n";
					}
				}
				// Otherwise, if "first" index is greater than 0, then this array access is out of bound
				else
				{
					Value* originIndex = findOriginOfPointer(*OI);
					
					if (dyn_cast<LoadInst>(originIndex))
						originIndex = ((LoadInst*)originIndex)->getOperand(0);
					
					errs() << "var1: " << originIndex << "\n";
					
					LLVMContext& context = this->M->getContext();
					std::vector<Value*> varNames2;
						
					ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(context), 0);
						
					varNames2.push_back(zero);
					varNames2.push_back(originIndex);
		
					MDNode* meta_GT = MDNode::get(context, varNames2);
					
					Instruction* callGTZero = this->checkGTLimit(zero, *OI);
					callGTZero->setMetadata("VarName", meta_GT);
				}					
			}
		}
		// bound checking for indices other than the "first" index
		else
		{
			if (ConstantInt *CI = dyn_cast<ConstantInt>(*OI)) // index in constant form
			{
				if (ArrayType *Aty = dyn_cast<ArrayType>(*GEPI))
				{
					int64_t index = CI->getSExtValue();
					int64_t numElements = Aty->getNumElements();

					errs() << "\nIndex (Constant): " << index << "\n";
					errs() << "numElements: " << numElements << "\n"; // number of elements in this part of array
				
					// index cannot go above number of elements - 1
					if (CI->getZExtValue() >= Aty->getNumElements())
					{
						errs() << "GEP index " << index << " is >= " << Aty->getNumElements() << "\n";
						errs() << "Compile-time analysis detected an out-of-bound access! Terminating...\n";
						exit(1);
					}
					else
					{
						errs() << "GEP index " << index << " passed compile-time check: " << index << " < " << Aty->getNumElements() << "\n";
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
					ConstantInt* arraySizeCI = ConstantInt::get(Type::getInt64Ty(context), Aty->getNumElements());

					Value* originIndex = findOriginOfPointer(*OI);
					
					errs() << "Origin Index: " << *originIndex << "\n";

					if (dyn_cast<LoadInst>(originIndex))
						originIndex = ((LoadInst*)originIndex)->getOperand(0);
				//	else
				//		errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";
					
					errs() << "var1: " << *originIndex << "\n";
					errs() << "var2: " << *arraySizeCI << "\n";

					std::vector<Value*> varNames1;
					std::vector<Value*> varNames2;
						
					varNames1.push_back(originIndex);
					varNames1.push_back(arraySizeCI);
					
					ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(context), 0);
						
					varNames2.push_back(zero);
					varNames2.push_back(originIndex);
						
					MDNode* meta_LT = MDNode::get(context, varNames1);
					MDNode* meta_GT = MDNode::get(context, varNames2);

					Instruction* callLTLimit = this->checkLTLimit(*OI, arraySizeCI);
					Instruction* callGTZero = this->checkGTLimit(zero, *OI);
					
					callLTLimit->setMetadata("VarName", meta_LT);
					callGTZero->setMetadata("VarName", meta_GT);
				}
			}
		}
	}

	errs() << "----------------------------------------------------------------------\n";

	return true;
}

