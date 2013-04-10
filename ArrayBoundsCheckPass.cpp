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
	return this->insertLessThanCheck(left, right);	
}

Instruction* ArrayBoundsCheckPass::insertLessThanCheck(Value* left, Value* right)
{
	this->checkNumber++;

	errs() << "-----------------------------------------------------------------------------------------------------------------\n";
	errs() << "   *** Inserting a Bound Check Instruction ***\n";
	errs() <<"   " <<  *left << "  <  " << *right << "\n";
	errs() << "-----------------------------------------------------------------------------------------------------------------\n";

	if (left->getType() != Type::getInt64Ty(this->M->getContext()))
	{
		left = llvm::CastInst::CreateIntegerCast(left, Type::getInt64Ty(this->M->getContext()), true, "ADD_CHECK", Inst);
	}

	if (right->getType() != Type::getInt64Ty(this->M->getContext()))
	{
		right = llvm::CastInst::CreateIntegerCast(right, Type::getInt64Ty(this->M->getContext()), true, "ADD_CHECK", Inst);
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
	errs() << "-----------------------------------------------------------------------------------------------------------------\n";
	errs() << " Number of checks inserted: " <<  this->checkNumber << "\n";
	errs() << "-----------------------------------------------------------------------------------------------------------------\n";
	return true;
}

bool ArrayBoundsCheckPass::runOnModule(Module& M)
{
	errs() << "#################################################################################################################\n";
	errs() << "	              	Array Bound Checks Insertion Pass          \n";
	errs() << "#################################################################################################################\n";

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
//		errs() << "-----------------------------------------------------------------------------------------------------------------\n";
//		errs() << "[GEP expression detected]: " << *currInst << "\n";

		checkGEP(CE, currInst);
	}

	return true;
}


// check array bounds of a GEP instruction or a GEP expression
// I think it is sufficient to check whether FIRST index of GEP instruction is greater than 0 (except in VLA case)
bool ArrayBoundsCheckPass::checkGEP(User* user, Instruction* currInst)
{
	errs() << "-----------------------------------------------------------------------------------------------------------------\n";
	errs() << "   [GEP instruction detected]: " << *currInst << "\n";
	errs() << "-----------------------------------------------------------------------------------------------------------------\n";
	
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
				errs() << "ERROR: origin pointer returned NULL. SHOULD NOT HAPPEN!!!\n";
			}
			
			//errs() << "Base Pointer Type: " << **GEPI << "\n";
			//errs() << "Base Pointer " << *basePointer << "\n";
			//errs() << "Origin Base Pointer: " << *originPointer << "\n";
		
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
						this->Inst = currInst;
						
						errs() << "VLA Detected1\n";
						errs() << "base pointer: " << *allocaInst << "\n";

						//errs() << "arr_index: " << arr_index << "\n";
						arr_index = firstIndex;
			
						Value* limit;
						Type* elementType = allocaInst->getType()->getElementType();

						if (elementType->isArrayTy()) // array name is being used as a pointer (ex. a[10]; a + 30;)
						{
							errs() << "** This GEP is not an array access. **\n";
							return true;

//							ArrayType *Aty = dyn_cast<ArrayType>(elementType);
//							int64_t numElements = Aty->getNumElements();
//							limit = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), numElements);
						}
						else
						{
							limit = allocaInst->getOperand(0);
						}

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
							errs() << "****************************************************************************\n";
							errs() << "       Array Access Violation detected!\n";
							errs() <<    CI->getSExtValue() << " is less than zero!!!\n";
							errs() << "       Program is terminating...\n";
							errs() << "****************************************************************************\n";
							exit(1);
						}
						
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
						this->Inst = currInst;
						
						// Insert runtime check 
						errs() << "VLA Chain Detected1\n";
						errs() << "origin pointer: " << *originPointer << "\n";
						
						arr_index = firstIndex;
						//errs() << "arr_index: " << arr_index << "\n";

						Value* firstOp = dyn_cast<User>(basePointer)->getOperand(1);
						Value* secondOp = &(**OI);

						if (firstOp->getType() != Type::getInt64Ty(this->M->getContext())) 
						{
							firstOp = llvm::CastInst::CreateIntegerCast(firstOp, Type::getInt64Ty(this->M->getContext()), true, "ADD_CHECK", currInst);
						}

						if (secondOp->getType() != Type::getInt64Ty(this->M->getContext()))
						{
							secondOp = llvm::CastInst::CreateIntegerCast(secondOp, Type::getInt64Ty(this->M->getContext()), true, "ADD_CHECK", currInst);
						}

						Value* index;

						if (dyn_cast<ConstantInt>(secondOp)->getSExtValue() == 0)
						{
							index = firstOp;
						}
						else
						{
							index = BinaryOperator::Create(Instruction::Add, firstOp, secondOp, "ADD_CHECK", currInst);
						}
						
						Value* limit;
						Type* elementType = allocaInst->getType()->getElementType();
						
						if (elementType->isArrayTy()) // array name is being used as a pointer (ex. a[10]; a + 30;)
						{
							errs() << "** This GEP is not an array access. **\n";
							return true;

							//ArrayType *Aty = dyn_cast<ArrayType>(elementType);
							//int64_t numElements = Aty->getNumElements();
							//limit = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), numElements);
						}
						else
						{
							limit = allocaInst->getOperand(0);
						}
						
						Value* originIndex = findOriginOfPointer(index);
						Value* originLimit = findOriginOfPointer(limit);

						if (originIndex == NULL)
							originIndex = index;
						
						if (originLimit == NULL)
							originLimit = limit;

						errs() << "index: " << *index << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "origin index: " << *originIndex << "\n";
						errs() << "origin limit: " << *originLimit << "\n";
						
						if (CI->getSExtValue() < 0)
						{
							errs() << "****************************************************************************\n";
							errs() << "     Array Access Violation detected!\n";
							errs() << CI->getSExtValue() << " is less than zero!!!\n";
							errs() << "     Program terminating...\n";
							errs() << "****************************************************************************\n";
							exit(1);
						}
						
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
						errs() << "** This GEP is not an array access. **\n";
						return true;
					}
				}		
				else
				{
					int64_t currentIndex = CI->getSExtValue();
					
					errs() << "Regular Array Detected\n";
					errs() << "Base Pointer: " << *(dyn_cast<AllocaInst>(basePointer)) << "\n";
						
					arr_size = 1;
					arr_index = currentIndex;
				}			
			}
			else // First index is in non-constant form
			{
				errs() << "First Index (Non-constant): " << **OI << "\n";
				index_all_const = false;
		
				// if there is only one index in GEP and Base Pointer is alloca, then this must be a VLA
				if ((OI+1) == user->op_end())
				{
					if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(basePointer))
					{
						this->Inst = currInst;
						
						// Insert runtime check 
						errs() << "VLA Detected2\n";
						errs() << "base pointer: " << *allocaInst << "\n";

						Value* originIndex = findOriginOfPointer(*OI);
						Value* index = *OI;
					
						Value* limit;
						Type* elementType = allocaInst->getType()->getElementType();

						if (elementType->isArrayTy())
						{
							errs() << "** This GEP is not an array access. **\n";
							return true;

							//ArrayType *Aty = dyn_cast<ArrayType>(elementType);
							//int64_t numElements = Aty->getNumElements();
							//limit = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), numElements);
						}
						else
						{
							limit = allocaInst->getOperand(0);
						}
						
						Value* originLimit = findOriginOfPointer(limit);

						if (originIndex == NULL)
							originIndex = *OI;

						if (originLimit == NULL)
							originLimit = limit;

						if (dyn_cast<LoadInst>(originIndex))
							originIndex = ((LoadInst*) originIndex)->getOperand(0);

						if (dyn_cast<LoadInst>(originLimit))
							originLimit = ((LoadInst*)originLimit)->getOperand(0);
						
						errs() << "index: " << *index << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "origin index: " << *originIndex << "\n";
						errs() << "origin limit: " << *originLimit << "\n";
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
						
						varNames1.push_back(negone);
						varNames1.push_back(originIndex);
						
						varNames2.push_back(originIndex);
						varNames2.push_back(originLimit);
						
						MDNode* meta_lowerBound = MDNode::get(context, varNames1);
						MDNode* meta_upperBound = MDNode::get(context, varNames2);

						Instruction* lowerBoundCheck = this->checkLessThan(negone, index);
						Instruction* upperBoundCheck = this->checkLessThan(index, limit);
					
						lowerBoundCheck->setMetadata("VarName", meta_lowerBound);
						upperBoundCheck->setMetadata("VarName", meta_upperBound);
					
						return true;
					}
					else if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(originPointer))
					{
						// Insert runtime check 
						errs() << "VLA Chain Detected2\n";

						errs() << "OriginPointer: " << *allocaInst << "\n";
						
						Value* firstOp = dyn_cast<User>(basePointer)->getOperand(1);
						Value* secondOp = &(**OI);

						if (firstOp->getType() != Type::getInt64Ty(this->M->getContext())) 
						{
							firstOp = llvm::CastInst::CreateIntegerCast(firstOp, Type::getInt64Ty(this->M->getContext()), true, "ADD_CHECK", currInst);
						}

						if (secondOp->getType() != Type::getInt64Ty(this->M->getContext()))
						{
							secondOp = llvm::CastInst::CreateIntegerCast(secondOp, Type::getInt64Ty(this->M->getContext()), true, "ADD_CHECK", currInst);
						}

						BinaryOperator* index = BinaryOperator::Create(Instruction::Add, firstOp, secondOp, "ADD_CHECK", currInst);

						Value* originIndex = findOriginOfPointer(*OI);
						
						Value* limit;
						Type* elementType = allocaInst->getType()->getElementType();

						if (elementType->isArrayTy())
						{
							errs() << "** This GEP is not an array access. **\n";
							return true;

							//ArrayType *Aty = dyn_cast<ArrayType>(elementType);
							//int64_t numElements = Aty->getNumElements();
							//limit = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), numElements);
						}
						else
						{
							limit = allocaInst->getOperand(0);
						}
						
						Value* originLimit = findOriginOfPointer(limit);

						if (originIndex == NULL)
							originIndex = *OI;

						if (originLimit == NULL)
							originLimit = limit;

						if (dyn_cast<LoadInst>(originIndex))
							originIndex = ((LoadInst*)originIndex)->getOperand(0);

						if (dyn_cast<LoadInst>(originLimit))
							originLimit = ((LoadInst*)originLimit)->getOperand(0);
					
						errs() << "index: " << *index << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "origin index: " << *originIndex << "\n";
						errs() << "origin limit: " << *originLimit << "\n";
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
						
						varNames1.push_back(negone);
						varNames1.push_back(originIndex);
						
						varNames2.push_back(originIndex);
						varNames2.push_back(originLimit);
					
						MDNode* meta_lowerBound = MDNode::get(context, varNames1);
						MDNode* meta_upperBound = MDNode::get(context, varNames2);

						Instruction* lowerBoundCheck = this->checkLessThan(negone, index);
						Instruction* upperBoundCheck = this->checkLessThan(index, limit);
					
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
				else // regular array iteration
				{
					index_all_const = false;
					errs() << "Regular Array Detected\n";
					errs() << "Base Pointer: " << *(dyn_cast<AllocaInst>(basePointer)) << "\n";
					
					arr_size = 1;
					arr_index = 0;
					var_index = *OI;
					
					errs() << "Variable index: " << *var_index;
				}					
			}
		}
		// bound checking for indices other than the "first" indexn
		else
		{
			errs() << "Regular Array Iteration\n";

			if (ConstantInt *CI = dyn_cast<ConstantInt>(*OI)) // index in constant form
			{
				if (ArrayType *Aty = dyn_cast<ArrayType>(*GEPI))
				{
					int64_t current_index = CI->getSExtValue();
					int64_t numElements = Aty->getNumElements();
					arr_size = arr_size * numElements;

					errs() << "this index: " << current_index << "\n";
					errs() << "this array size: " << numElements << "\n";
					
					if (index_all_const)
					{
						arr_index = (arr_index * numElements) + current_index;
						errs() << "Cumulative array index: " << arr_index << "\n";
					}
					else
					{
						ConstantInt* secondOp = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), numElements);
						ConstantInt* thirdOp = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), current_index);
						
						if (var_index->getType() != Type::getInt64Ty(this->M->getContext()))
						{
							var_index = llvm::CastInst::CreateIntegerCast(var_index, Type::getInt64Ty(this->M->getContext()), true, "ADD_CHECK", currInst);
						}

						var_index = BinaryOperator::Create(Instruction::Mul, var_index, secondOp, "ADD_CHECK", currInst);

						if (current_index != 0)
							var_index = BinaryOperator::Create(Instruction::Add, var_index, thirdOp, "ADD_CHECK", currInst);
					
						errs() << "Cumulative array index: " << *var_index << "\n";
					}
					
					errs() << "cumulutive array size: " << arr_size << "\n";
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

					errs() << "this index: " << *current_index << "\n";
					errs() << "this array size: " << numElements << "\n";
					
					this->Inst = currInst;

					Value* originIndex = findOriginOfPointer(*OI);
					
					if (originIndex == NULL)
						originIndex = *OI;

					if (dyn_cast<LoadInst>(originIndex))
						originIndex = ((LoadInst*)originIndex)->getOperand(0);
					
					if (var_index == NULL)
					{
						if (arr_index == 0)
						{
								var_index = current_index;
								
								errs() << "cumulative array index: " << *var_index << "\n";
								errs() << "cumulative array size: " << arr_size << "\n";
								continue;
						}					
						else
						{
								var_index = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), arr_index*numElements);
						}		
					}
					else
					{
						ConstantInt* secondOp = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), numElements);
								
						if (var_index->getType() != Type::getInt64Ty(this->M->getContext()))
						{
							var_index = llvm::CastInst::CreateIntegerCast(var_index, Type::getInt64Ty(this->M->getContext()), true, "ADD_CHECK", currInst);
						}

						var_index = BinaryOperator::Create(Instruction::Mul, var_index, secondOp, "ADD_CHECK", currInst);
					}
					
					var_index = BinaryOperator::Create(Instruction::Add, var_index, current_index, "ADD_CHEK", currInst);
						
					errs() << "cumulative array index: " << *var_index << "\n";
					errs() << "cumulative array size: " << arr_size << "\n";
				}
			}
		}
		errs() << "\n";
	}
						
	if (arr_size == 0)
	{
			errs() << "** This GEP involves a global array whose size cannot be determined at compile time. **\n";
			return true;
	}

	errs() << "---------------------- GEP Statistics: -----------------------\n";
	errs() << "Cumulative Index: ";

	if (index_all_const)
		errs() << arr_index << "\n";
	else
		errs() << *var_index << "\n";
   
	errs() << "Cumulative Array Size: " << arr_size << "\n";

	ConstantInt* arr_size_CI = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), arr_size);	

	std::vector<Value*> varNames1;
	std::vector<Value*> varNames2;

	if (index_all_const)
	{
		if (arr_index < 0)
		{
			errs() << "*****************************************************************************\n";
			errs() << "  Array Access Violation detected!\n";
			errs() << "  array index: " << arr_index << " is less than zero!  \n";
			errs() << "  Program terminating...\n";
			errs() << "*****************************************************************************\n";
			exit(1);
		}
		else if (arr_index > arr_size)
		{
			errs() << "*****************************************************************************\n";
			errs() << "  Array Access Violation detected!\n";
			errs() << "  array index: " << arr_index << " is greater than or equal to: " << arr_size << "\n";
			errs() << "  Program terminating...\n";
			errs() << "******************************************************************************\n";
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

	errs() << "-----------------------------------------------------------------------------------------------------------------\n";

	return true;
}
