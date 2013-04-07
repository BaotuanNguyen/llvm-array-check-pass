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
#ifndef	__NOT_VERBOSE__
					errs() << "Left index " << leftInt << " is greater than " << rightInt << "\n";
					errs() << "Compile-time analysis detected an out-of-bound access! Terminating...\n";
#endif
					exit(1);
			}
			else
			{
#ifndef	__NOT_VERBOSE__
					errs() << "Bound Check: " << leftInt << " < " << rightInt << " passed at compile-time\n";
#endif
					return NULL;
			}
		}
	}

	return this->insertLessThanCheck(left, right);	
}

Instruction* ArrayBoundsCheckPass::insertLessThanCheck(Value* left, Value* right)
{
	this->checkNumber++;
	
	///create types of the arguments
	LLVMContext& context = this->M->getContext();
	
	if (left->getType() == Type::getInt32Ty(context))
		left = llvm::CastInst::CreateIntegerCast(left, Type::getInt64Ty(context), true, "", Inst);
	
	if (right->getType() == Type::getInt32Ty(context))
		right = llvm::CastInst::CreateIntegerCast(right, Type::getInt64Ty(context), true, "", Inst);
	
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

	this->checkNumber = 0;
	this->M = &M;
	this->negone = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), -1);

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
#ifndef	__NOT_VERBOSE__
	errs() << "-------------------------------------------------------\n";
#endif
	errs() << "Number of checks inserted: " <<  this->checkNumber << "\n";
#ifndef	__NOT_VERBOSE__
	errs() << "-------------------------------------------------------\n";
#endif
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
#ifndef	__NOT_VERBOSE__
			errs() << "----------------------------------------------------------------------\n";
			errs() << "[GEP instruction detected]: " << *GEP << "\n";
#endif
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
#ifndef	__NOT_VERBOSE__
		errs() << "----------------------------------------------------------------------\n";
		errs() << "[GEP expression detected]: " << *currInst << "\n";
#endif

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
#ifndef	__NOT_VERBOSE__
				errs() << "origin pointer returned NULL. SHOULD NOT HAPPEN!!!\n";
#endif
			}
			
#ifndef	__NOT_VERBOSE__
			errs() << "Base Pointer Type: " << **GEPI << "\n";
			errs() << "Base Pointer " << *basePointer << "\n";
			errs() << "Origin Base Pointer: " << *originPointer << "\n";
#endif
		
			OI++; // now OI points to the first index position
			
			// second operand contains "first" index
			if (ConstantInt *CI = dyn_cast<ConstantInt>(*OI))
			{
				int64_t firstIndex = CI->getSExtValue();
#ifndef	__NOT_VERBOSE__
				errs() << "\nFirst Index (Constant): " << firstIndex << "\n";
#endif
			
				// if there is only one index in GEP and Base Pointer is alloca, then this must be a VLA
				if ((OI+1) == user->op_end())
				{
					if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(basePointer))
					{
#ifndef	__NOT_VERBOSE__
						errs() << "VLA Detected1\n";
#endif
						this->Inst = currInst;
						
						Value* limit = allocaInst->getOperand(0);
						Value* originLimit = findOriginOfPointer(limit);

						if (dyn_cast<LoadInst>(originLimit))
							originLimit = ((LoadInst*) originLimit)->getOperand(0);
				//		else
				//			errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";

#ifndef	__NOT_VERBOSE__
						errs() << "index: " << *CI << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "var1: " << *CI << "\n";
						errs() << "var2: " << *originLimit << "\n";
#endif
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
					
						varNames1.push_back(negone);
						varNames1.push_back(CI);
						
						varNames2.push_back(CI);
						varNames2.push_back(originLimit);
						
						MDNode* meta_lowerBound = MDNode::get(context, varNames1);
						MDNode* meta_upperBound = MDNode::get(context, varNames2);

						Instruction* lowerBoundCheck = this->checkLessThan(negone, CI);
						Instruction* upperBoundCheck = this->checkLessThan(CI, limit);
						
						if (lowerBoundCheck)
							lowerBoundCheck->setMetadata("VarName", meta_lowerBound);
						
						if (upperBoundCheck)
							upperBoundCheck->setMetadata("VarName", meta_upperBound);
					}
					else if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(originPointer))
					{
						// Insert runtime check 
#ifndef	__NOT_VERBOSE__
						errs() << "VLA Chain Detected1\n";
#endif
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
#ifndef	__NOT_VERBOSE__
						errs() << "Origin Pointer: " << *originPointer << "\n";
						
						errs() << "index: " << *index << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "var1: " << *index << "\n";
						errs() << "var2: " << *originLimit << "\n";
#endif
						
						LLVMContext& context = this->M->getContext();
						std::vector<Value*> varNames1;
						std::vector<Value*> varNames2;
						
						varNames1.push_back(negone);
						varNames1.push_back(index);
						
						varNames2.push_back(index);
						varNames2.push_back(limit);
						
						MDNode* meta_lowerBound = MDNode::get(context, varNames1);
						MDNode* meta_upperBound = MDNode::get(context, varNames2);

						Instruction* lowerBoundCheck = this->checkLessThan(negone, index);
						Instruction* upperBoundCheck = this->checkLessThan(index, limit);
						
						if (lowerBoundCheck)
							lowerBoundCheck->setMetadata("VarName", meta_lowerBound);
						
						if (upperBoundCheck)
							upperBoundCheck->setMetadata("VarName", meta_upperBound);
					}
					else
					{
#ifndef	__NOT_VERBOSE__
						errs() << "This GEP is not an array access\n";
#endif
					}
				}		
				// Otherwise, if "first" index is greater than 0, then this array access is out of bound
				else
				{
					ConstantInt* one = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), 1); // index should be < 1
					this->checkLessThan(CI, one); // guaranteed to be checked at compile-time;
				}			
			}
			else // First index is in non-constant form
			{
#ifndef	__NOT_VERBOSE__
				errs() << "\nFirst Index (Non-constant): " << **OI << "\n";
#endif
		
				// if there is only one index in GEP and Base Pointer is alloca, then this must be a VLA
				if ((OI+1) == user->op_end())
				{
					if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(basePointer))
					{
						// Insert runtime check 
#ifndef	__NOT_VERBOSE__
						errs() << "VLA Detected2\n";
#endif
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
						
#ifndef	__NOT_VERBOSE__
						errs() << "index: " << **OI << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "var1: " << *originIndex << "\n";
						errs() << "var2: " << *originLimit << "\n";
#endif
						
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
					
						if (lowerBoundCheck)
							lowerBoundCheck->setMetadata("VarName", meta_lowerBound);
						if (upperBoundCheck)
							upperBoundCheck->setMetadata("VarName", meta_upperBound);
					}
					else if (AllocaInst* allocaInst = dyn_cast<AllocaInst>(originPointer))
					{
						// Insert runtime check 
#ifndef	__NOT_VERBOSE__
						errs() << "VLA Chain Detected2\n";
#endif
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
#ifndef	__NOT_VERBOSE__
						errs() << "Origin Pointer: " << *originPointer << "\n";
						
						errs() << "index: " << *index << "\n";
						errs() << "limit: " << *limit << "\n";
						
						errs() << "var1: " << *originIndex << "\n";
						errs() << "var2: " << *originLimit << "\n";
#endif
						
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
					
						if (lowerBoundCheck)
							lowerBoundCheck->setMetadata("VarName", meta_lowerBound);
						if (upperBoundCheck)
							upperBoundCheck->setMetadata("VarName", meta_upperBound);
					}
					else
					{
#ifndef	__NOT_VERBOSE__
						errs() << "This GEP is not an array access\n";
#endif
					}
				}
				// Otherwise, if "first" index is greater than 0, then this array access is out of bound
				else
				{
					Value* originIndex = findOriginOfPointer(*OI);
					ConstantInt* one = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), 1); // index should be < 1
					
					if (dyn_cast<LoadInst>(originIndex))
						originIndex = ((LoadInst*)originIndex)->getOperand(0);
					
#ifndef	__NOT_VERBOSE__
					errs() << "var1: " << originIndex << "\n";
#endif
					
					LLVMContext& context = this->M->getContext();
					std::vector<Value*> varNames;
						
					varNames.push_back(originIndex);
					varNames.push_back(one);
		
					MDNode* meta_upperBound = MDNode::get(context, varNames);
					
					Instruction* upperBoundCheck = this->checkLessThan(*OI, one);
					
					if (upperBoundCheck)
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
					int64_t index = CI->getSExtValue();
					int64_t numElements = Aty->getNumElements();

#ifndef	__NOT_VERBOSE__
					errs() << "\nIndex (Constant): " << index << "\n";
					errs() << "numElements: " << numElements << "\n"; // number of elements in this part of array
#endif
	
					ConstantInt* arraySize = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), numElements);
	
					this->checkLessThan(CI, arraySize); // guaranteed to be checked at compile-time
				}
			}
			else // index is in non-constant form
			{
				if (ArrayType *Aty = dyn_cast<ArrayType>(*GEPI))
				{
#ifndef	__NOT_VERBOSE__
					errs() << "\nIndex (Non-constant): " << **OI << "\n";
					errs() << "NumElements: " << Aty->getNumElements() << "\n";
#endif
			
					this->Inst = currInst;
					LLVMContext& context = this->M->getContext();
					ConstantInt* arraySizeCI = ConstantInt::get(Type::getInt64Ty(context), Aty->getNumElements());

					Value* originIndex = findOriginOfPointer(*OI);
					
#ifndef	__NOT_VERBOSE__
					errs() << "Origin Index: " << *originIndex << "\n";
#endif

					if (dyn_cast<LoadInst>(originIndex))
						originIndex = ((LoadInst*)originIndex)->getOperand(0);
				//	else
				//		errs() << "ORIGIN OF RANGE VARIABLE SHOULD BE LOAD INSTRUCTION!\n";
					
#ifndef	__NOT_VERBOSE__
					errs() << "var1: " << *originIndex << "\n";
					errs() << "var2: " << *arraySizeCI << "\n";
#endif

					std::vector<Value*> varNames1;
					std::vector<Value*> varNames2;
						
					varNames1.push_back(negone);
					varNames1.push_back(originIndex);
					
					varNames2.push_back(originIndex);
					varNames2.push_back(arraySizeCI);
						
					MDNode* meta_lowerBound = MDNode::get(context, varNames1);
					MDNode* meta_upperBound = MDNode::get(context, varNames2);

					Instruction* lowerBoundCheck = this->checkLessThan(negone, *OI);
					Instruction* upperBoundCheck = this->checkLessThan(*OI, arraySizeCI);
				
					if (lowerBoundCheck)
						lowerBoundCheck->setMetadata("VarName", meta_lowerBound);
					if (upperBoundCheck)
						upperBoundCheck->setMetadata("VarName", meta_upperBound);
				}
			}
		}
	}

#ifndef	__NOT_VERBOSE__
	errs() << "----------------------------------------------------------------------\n";
#endif

	return true;
}

