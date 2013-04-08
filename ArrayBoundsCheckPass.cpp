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

Instruction* ArrayBoundsCheckPass::checkLessThan(Value* left, Value* right)
{
	if (ConstantInt* leftCI = dyn_cast<ConstantInt>(left))
	{
		if (ConstantInt* rightCI = dyn_cast<ConstantInt>(right))
		{
			int64_t leftInt = leftCI->getSExtValue();
			int64_t rightInt = rightCI->getSExtValue();

			if (leftInt >= rightInt)
			{
					errs() << "Left index " << leftInt << " is greater than " << rightInt << "\n";
					errs() << "Compile-time analysis detected an out-of-bound access! Terminating...\n";
					exit(1);
			}
			else
			{
					errs() << "Bound Check: " << leftInt << " < " << rightInt << " passed at compile-time\n";
					return NULL;
			}
		}
	}

	return this->insertLessThanCheck(left, right);	
}

Instruction* ArrayBoundsCheckPass::insertLessThanCheck(Value* left, Value* right)
{
	this->checkNumber++;

	errs() << "*********************** Inserting A LT CALL: *************************\n";
	errs() << "	op1: " << *left << "  <  op2: " << *right << "\n";
	errs() << "**********************************************************************\n";

	if (left->getType() == Type::getInt32Ty(this->M->getContext()))
	{
		left = llvm::CastInst::CreateIntegerCast(left, Type::getInt64Ty(this->M->getContext()), true, "", Inst);
		errs() << "yes\n";
	}
	if (right->getType() == Type::getInt32Ty(this->M->getContext()))
	{
		right = llvm::CastInst::CreateIntegerCast(right, Type::getInt64Ty(this->M->getContext()), true, "", Inst);
		errs() << "no\n";
	}
	
	///create vector with values which containts arguments values
	std::vector<Value*> argValuesV;
	argValuesV.push_back(left);
	argValuesV.push_back(right);
	
	///create array ref to vector or arguments
	ArrayRef<Value*> argValuesA(argValuesV);

	///create call in code
	CallInst* allocaCall = CallInst::Create(this->checkLessThanFunction, argValuesA, "", Inst);
	
	allocaCall->setCallingConv(CallingConv::C);
	allocaCall->setTailCall(false);

	return allocaCall;
}

bool ArrayBoundsCheckPass::doInitialization(Module& M)
{
	std::vector<Type*> argTypes;

	this->M = &M;
	this->checkNumber = 0;

	Type* voidTy = Type::getVoidTy(M.getContext());
	Type* intTy = Type::getInt64Ty(M.getContext());
	//Type* charPtrTy = Type::getInt8PtrTy(M.getContext());

	argTypes.push_back(intTy);
	argTypes.push_back(intTy);

	//declared functions type calls
	ArrayRef<Type*> argArray(argTypes);
	
	FunctionType* checkLessThanFunctionType = FunctionType::get(voidTy, argArray, false);

	//create functions
	M.getOrInsertFunction("checkLessThan", checkLessThanFunctionType);
	
	//store Value to function
	this->checkLessThanFunction = M.getFunction("checkLessThan");

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
	bool index_all_const = true;
	int64_t arr_size = 0;
	int64_t arr_index = 1;
	Value* var_index = NULL;
	this->Inst = currInst;
	
	ConstantInt* negone = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), -1);
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
				originPointer = basePointer;
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
						
						errs() << "arr_index: " << arr_index << "\n";
						arr_index = firstIndex;

						Value* limit = allocaInst->getOperand(0);
						Value* originLimit = findOriginOfPointer(limit);

						if (originLimit == NULL)
							originLimit = limit;

						if (dyn_cast<LoadInst>(originLimit))
							originLimit = ((LoadInst*) originLimit)->getOperand(0);

						errs() << "index: " << *CI << "\n";
						errs() << "limit: " << *limit << "\n";
						errs() << "originIndex: " << *CI << "\n";
						errs() << "originLimit: " << *originLimit << "\n";

						if (CI->getSExtValue() < 0)
						{
							errs() << "AN OUT OF ARRAY BOUND ACCESS DETECTED!!\n";
							errs() << CI->getSExtValue() << " is less than zero!!!!!\n";
							exit(1);
						}
						
						errs() << "var1: " << *CI << "\n";
						errs() << "var2: " << *originLimit << "\n";
						
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
					
						varNames2.push_back(CI);
						varNames2.push_back(originLimit);
						
						MDNode* meta_upperBound = MDNode::get(this->M->getContext(), varNames2);

						Instruction* upperBoundCheck = this->checkLessThan(CI, limit);
						
						upperBoundCheck->setMetadata("VarName", meta_upperBound);
						
						return true;
					}
					else if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(originPointer))
					{
						// Insert runtime check 
						errs() << "VLA Chain Detected1\n";
						this->Inst = currInst;
						
						arr_index = firstIndex;
						errs() << "arr_index: " << arr_index << "\n";

						Value* firstOp = dyn_cast<User>(basePointer)->getOperand(1);
						Value* secondOp = &(**OI);

						if (!(firstOp->getType() == Type::getInt32Ty(this->M->getContext())) || !(secondOp->getType() == Type::getInt32Ty(this->M->getContext())))
						{
							firstOp = llvm::CastInst::CreateIntegerCast(firstOp, Type::getInt64Ty(this->M->getContext()), true, "", currInst);
							secondOp = llvm::CastInst::CreateIntegerCast(secondOp, Type::getInt64Ty(this->M->getContext()), true, "", currInst);
						}

						BinaryOperator* index = BinaryOperator::Create(Instruction::Add, firstOp, secondOp, "", currInst);

						Value* limit = allocaInst->getOperand(0);
						Value* originIndex = findOriginOfPointer(index);
						Value* originLimit = findOriginOfPointer(limit);

						if (originLimit == NULL)
							originLimit = limit;

						if (originIndex == NULL)
							originIndex = index;

						errs() << "index: " << *index << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "origin index: " << *originIndex << "\n";
						errs() << "origin limit: " << *originLimit << "\n";
						
						if (CI->getSExtValue() < 0)
						{
							errs() << "AN OUT OF ARRAY BOUND ACCESS DETECTED!!\n";
							errs() << CI->getSExtValue() << " is less than zero!!!!!\n";
							exit(1);
						}
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
						std::vector<Value*> varNames2;
						
						varNames2.push_back(originIndex);
						varNames2.push_back(originLimit);
						
						MDNode* meta_upperBound = MDNode::get(context, varNames2);

						Instruction* upperBoundCheck = this->checkLessThan(index, limit);
						
						upperBoundCheck->setMetadata("VarName", meta_upperBound);
					
						return true;
					}
					else
					{
						errs() << "This GEP is not an array access\n";
						return true;
					}
				}		
				else
				{
					arr_size = 1;
					errs() << "arr_size: " << arr_size << "\n";
						
					int64_t currentIndex = CI->getSExtValue();
					arr_index = currentIndex;

					errs() << "arr_index: " << arr_index << "\n";
				}			
			}
			else // First index is in non-constant form
			{
				errs() << "\nFirst Index (Non-constant): " << **OI << "\n";
				index_all_const = false;

		
				// if there is only one index in GEP and Base Pointer is alloca, then this must be a VLA
				if ((OI+1) == user->op_end())
				{
					if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(basePointer))
					{
						// Insert runtime check 
						errs() << "VLA Detected2\n";
						this->Inst = currInst;

						Value* originIndex = findOriginOfPointer(*OI);
						Value* index = *OI;
						Value* limit = allocaInst->getOperand(0);
						Value* originLimit = findOriginOfPointer(limit);

						if (originIndex == NULL)
							originIndex = *OI;

						if (originLimit == NULL)
							originLimit = limit;

						if (dyn_cast<LoadInst>(originIndex))
							originIndex = ((LoadInst*) originIndex)->getOperand(0);
				//		else
				//			errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";

						if (dyn_cast<LoadInst>(originLimit))
							originLimit = ((LoadInst*)originLimit)->getOperand(0);
				//		else
				//			errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";
						
						errs() << "index: " << *index << "\n";
						errs() << "index: " << **OI << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "var1: " << *originIndex << "\n";
						errs() << "var2: " << *originLimit << "\n";
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
						
						varNames1.push_back(negone);
						varNames1.push_back(originIndex);
						
						varNames2.push_back(originIndex);
						varNames2.push_back(originLimit);
						
						MDNode* meta_lowerBound = MDNode::get(context, varNames1);
						MDNode* meta_upperBound = MDNode::get(context, varNames2);

						Instruction* lowerBoundCheck = this->checkLessThan(negone, *OI);
						Instruction* upperBoundCheck = this->checkLessThan(*OI, limit);
					
						lowerBoundCheck->setMetadata("VarName", meta_lowerBound);
						upperBoundCheck->setMetadata("VarName", meta_upperBound);
					
						return true;
					}
					else if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(originPointer))
					{
						// Insert runtime check 
						errs() << "VLA Chain Detected2\n";
						this->Inst = currInst;
						
						errs() << "currInst: " << *currInst << "\n";

						Value* firstOp = dyn_cast<User>(basePointer)->getOperand(1);
						Value* secondOp = &(**OI);

						if (!(firstOp->getType() == Type::getInt32Ty(this->M->getContext())) || !(secondOp->getType() == Type::getInt32Ty(this->M->getContext())))
						{
							firstOp = llvm::CastInst::CreateIntegerCast(firstOp, Type::getInt64Ty(this->M->getContext()), true, "", currInst);
							secondOp = llvm::CastInst::CreateIntegerCast(secondOp, Type::getInt64Ty(this->M->getContext()), true, "", currInst);
						}

						BinaryOperator* index = BinaryOperator::Create(Instruction::Add, firstOp, secondOp, "", currInst);

						Value* originIndex = findOriginOfPointer(*OI);
						Value* limit = allocaInst->getOperand(0);
						Value* originLimit = findOriginOfPointer(limit);

						if (originIndex == NULL)
							originIndex = *OI;

						if (originLimit == NULL)
							originLimit = limit;

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
						
						errs() << "origin index: " << *originIndex << "\n";
						errs() << "origin limit: " << *originLimit << "\n";
						errs() << "var1: " << *originIndex << "\n";
						errs() << "var2: " << *originLimit << "\n";
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
						
						varNames1.push_back(negone);
						varNames1.push_back(originIndex);
						
						varNames2.push_back(originIndex);
						varNames2.push_back(originLimit);
					
						MDNode* meta_lowerBound = MDNode::get(context, varNames1);
						MDNode* meta_upperBound = MDNode::get(context, varNames2);

						Instruction* lowerBoundCheck = this->checkLessThan(negone, *OI);
						Instruction* upperBoundCheck = this->checkLessThan(*OI, limit);
					
						lowerBoundCheck->setMetadata("VarName", meta_lowerBound);
						upperBoundCheck->setMetadata("VarName", meta_upperBound);
						
						return true;
					}
					else
					{
						errs() << "This GEP is not an array access\n";
						return true;
					}
				}
				else
				{
					arr_size = 1;
					errs() << "arr_size: " << arr_size << "\n";

					var_index = *OI;
					errs() << "var_index: " << *var_index;
					
					Value* originIndex = findOriginOfPointer(*OI);
					ConstantInt* one = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), 1); // index should be < 1
					
					if (dyn_cast<LoadInst>(originIndex))
						originIndex = ((LoadInst*)originIndex)->getOperand(0);
					
					errs() << "var1: " << originIndex << "\n";
					
					LLVMContext& context = this->M->getContext();
					std::vector<Value*> varNames;
						
					varNames.push_back(originIndex);
					varNames.push_back(one);
		
					MDNode* meta_upperBound = MDNode::get(context, varNames);
					
					Instruction* upperBoundCheck = this->checkLessThan(*OI, one);
					
					upperBoundCheck->setMetadata("VarName", meta_upperBound);
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
					int64_t current_index = CI->getSExtValue();
					int64_t numElements = Aty->getNumElements();
					arr_size = arr_size * numElements;

					errs() << "current_index: " << current_index << "\n";
					errs() << "num elements: " << numElements << "\n";
					errs() << "arr_size: " << arr_size << "\n";
					errs() << "ar_index: " << arr_index << "\n";

					if (index_all_const)
					{
						arr_index = (arr_index * numElements) + current_index;
						errs() << "arr_index: " << arr_index << "\n";
					}
					else
					{
						ConstantInt* secondOp = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), numElements);
						ConstantInt* thirdOp = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), current_index);
						
						if (!(var_index->getType() == Type::getInt32Ty(this->M->getContext())))
						{
							var_index = llvm::CastInst::CreateIntegerCast(var_index, Type::getInt64Ty(this->M->getContext()), true, "", currInst);
						}

						var_index = BinaryOperator::Create(Instruction::Mul, var_index, secondOp, "", currInst);
						var_index = BinaryOperator::Create(Instruction::Add, var_index, thirdOp, "", currInst);
					
						errs() << "arr_index: " << *var_index << "\n";
					}
				}
			}
			else // index is in non-constant form
			{
				if (ArrayType *Aty = dyn_cast<ArrayType>(*GEPI))
				{
					index_all_const = false;

					Value* current_index = *OI;
					int64_t numElements = Aty->getNumElements();
					arr_size = arr_size * numElements;

					errs() << "current_index: " << *current_index << "\n";
					errs() << "num elements: " << numElements << "\n";
					errs() << "arr_size: " << arr_size << "\n";
					errs() << "arr_index: " << arr_index << "\n";
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

					if (var_index == NULL)
					{
						var_index = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), arr_index*numElements);
					}
					else
					{
						ConstantInt* secondOp = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), numElements);
								
						if (!(var_index->getType() == Type::getInt32Ty(this->M->getContext())))
						{
							var_index = llvm::CastInst::CreateIntegerCast(var_index, Type::getInt64Ty(this->M->getContext()), true, "", currInst);
						}

						var_index = BinaryOperator::Create(Instruction::Mul, var_index, secondOp, "", currInst);
					}
					
					var_index = BinaryOperator::Create(Instruction::Add, var_index, current_index, "", currInst);
						
					errs() << "arr_index: " << *var_index << "\n";
				}
			
			}
		}
		errs() << "\n";
	}

	errs() << "***GEP Result: arr_index: " << arr_index << " var_index: " << *var_index << "arr_size: " << arr_size << "***\n";

	ConstantInt* arr_size_CI = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), arr_size);	

	std::vector<Value*> varNames1;
	std::vector<Value*> varNames2;

	if (index_all_const)
	{
		if (arr_index < 0)
		{
			errs() << "index: " << arr_index << " is less than zero. Terminating...\n";
			exit(1);
		}
		else if (arr_index > arr_size)
		{
			errs() << "index: " << arr_index << " is greater than or equal to: " << arr_size << ".  Terminating...\n";
			exit(1);
		}

		return true;
	}

	Value* origin_index = findOriginOfPointer(var_index);
					
	if (origin_index == NULL)
		origin_index = var_index;

	varNames1.push_back(negone);
	varNames1.push_back(origin_index);

	varNames2.push_back(origin_index);
	varNames2.push_back(arr_size_CI);	

	Instruction* lowerBoundCheck = this->checkLessThan(negone, var_index);
	Instruction* upperBoundCheck = this->checkLessThan(var_index, arr_size_CI);

	MDNode* meta_lowerBound = MDNode::get(this->M->getContext(), varNames1);
	MDNode* meta_upperBound = MDNode::get(this->M->getContext(), varNames2);

	lowerBoundCheck->setMetadata("VarName", meta_lowerBound);
	upperBoundCheck->setMetadata("VarName", meta_upperBound);

	errs() << "----------------------------------------------------------------------\n";

	return true;
}

