#include "EffectGenPassMore.h"

using namespace llvm;

char EffectGenPassMore::ID = 0;
static RegisterPass<EffectGenPassMore> C("effect-gen-more", "Effect Generation MORE", false, false);

bool EffectGenPassMore::runOnModule(Module& M)
{
	this->M = &M;

	errs() << "\n#########################################################################\n";
	errs() << "   Generating Effects\n";
	errs() << "###########################################################################\n";

	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i)
	{
		Function* func = &(*i);
		runOnFunction(&(*func));
	}
	
	errs() << "\n########################################################################\n";
	errs() << "   DONE\n";
	errs() << "##########################################################################\n";

	return false;
}

bool EffectGenPassMore::runOnFunction(Function* F)
{
//	changed = true;
	
//	while (changed)
//	{
//		changed = false;

		for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
		{
			runOnBasicBlock(i);
		}
//	}
	return true;
}

bool EffectGenPassMore::runOnBasicBlock(BasicBlock* BB)
{
//	errs() << "Entering basic block\n";

		LLVMContext& context = this->M->getContext();
		ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(context), 0);
		
		MDString* unchangedString = MDString::get(context, "UNCHANGED");
		MDString* incrementString = MDString::get(context, "INCREMENT");
		MDString* decrementString = MDString::get(context, "DECREMENT");
		MDString* changedString = MDString::get(context, "CHANGED");
        
		for (BasicBlock::iterator i = BB->begin(), e = BB->end(); i != e; ++i) 
		{
				Instruction *inst = &*i;

//				errs() << "inst: " << *inst << "\n";

				if (LoadInst *LI = dyn_cast<LoadInst>(inst)) 
				{
					//errs() << "Load: " << *LI << "\n";
					//errs() << "OP: " << *(LI->getOperand(0)) << "\n";

					// can't do copy propattion on global variables because they cannnot have metadata
					if (dyn_cast<GlobalVariable>(LI->getOperand(0)))
					{
							generateMetadata(unchangedString, LI->getOperand(0), zero, LI, M);
							continue;
					}

					if (dyn_cast<AllocaInst>(LI->getOperand(0)))
					{
						MDNode* effect = dyn_cast<Instruction>(LI->getOperand(0))->getMetadata("EFFECT");
					
						if (effect == NULL)
						{
							generateMetadata(unchangedString, LI->getOperand(0), zero, LI, M);
						}
						else if (dyn_cast<MDString>(effect->getOperand(0))->getString().equals("CHANGED"))
						{
							generateMetadata(unchangedString, LI->getOperand(0), zero, LI, M);
						}
						else
						{
							MDString* str = dyn_cast<MDString>(effect->getOperand(0));
							Value* var1 = effect->getOperand(1);
							Value* var2 = effect->getOperand(2);
							generateMetadata(str, var1, var2, LI, M);
						}

						continue;
					}
				}
				
				if (StoreInst *SI = dyn_cast<StoreInst>(inst)) 
				{
//					errs() << "Store: " << *SI << "\n";
//					errs() << "OP: " << *(SI->getOperand(1)) << "\n";
					
					// can't do copy propattion on global variables because they cannnot have metadata
					if (dyn_cast<GlobalVariable>(SI->getOperand(1)))
					{
						continue;
					}

					if (dyn_cast<AllocaInst>(SI->getOperand(1)))
					{
						if (dyn_cast<Constant>(SI->getOperand(0)))
						{					
							generateMetadata(changedString, NULL, NULL, dyn_cast<Instruction>(SI->getOperand(1)), M);
							
							// kill all copies of this variable
							for (BasicBlock::iterator i = BB->begin(), e = BB->end(); i != e; ++i) 
							{
								Instruction* currInst = &(*i);

								if (SI->getOperand(1) == currInst)
								{
									generateMetadata(changedString, NULL, NULL, currInst, M);
									continue;
								}

								if (dyn_cast<AllocaInst>(currInst))
								{
									//errs() << "Alloca: " << *currInst << "\n";

									MDNode* effect = currInst->getMetadata("EFFECT");
									
									if (effect == NULL)
									{
//										errs() << "Effect NULL\n";
										continue;
									}

									if (dyn_cast<MDString>(effect->getOperand(0))->getString().equals("CHANGED"))
									{
//										errs() << "Effect Changed\n";
										continue;
									}

									if (effect->getOperand(1) == SI->getOperand(1))
									{
//										errs() << "Effect op1: " << *(effect->getOperand(1)) << " op2: " << *(SI->getOperand(1)) << "\n";
										generateMetadata(changedString, NULL, NULL, currInst, M);
										continue;
									}								
								}
							}
						}
						else if (Instruction* op1 = dyn_cast<Instruction>(SI->getOperand(0)))
						{
							MDNode* effect = op1->getMetadata("EFFECT");

							/*
							if (!(op1->hasName())) // can't do anything about temporary..
							{
								generateMetadata(changedString, NULL, NULL, dyn_cast<Instruction>(SI->getOperand(1)), M);
								continue;
							}*/

							if (effect == NULL)
							{
								ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(this->M->getContext()), 0);
								generateMetadata(unchangedString, SI->getOperand(0), zero, dyn_cast<Instruction>(SI->getOperand(1)), M);
								continue;
							}
							
							if (SI->getOperand(1) == effect->getOperand(1))
							{
								generateMetadata(changedString, NULL, NULL, dyn_cast<Instruction>(SI->getOperand(1)), M);
								continue;
							}
							
							MDString* str = dyn_cast<MDString>(effect->getOperand(0));
							Value* var1 = effect->getOperand(1);
							Value* var2 = effect->getOperand(2);
							generateMetadata(str, var1, var2, dyn_cast<Instruction>(SI->getOperand(1)), M);
						}
						
						continue;
					}
				}
				
				if (CastInst* CI = dyn_cast<CastInst>(inst)) 
				{
//					errs() << "Inspecting Cast Instruction\n";
					
					if (dyn_cast<Argument>(CI->getOperand(0)))
					{
						generateMetadata(changedString, NULL, NULL, CI, M);
						continue;
					}

					Instruction* operand1 = dyn_cast<Instruction>(CI->getOperand(0));

//					errs() << "operand: " << *operand1 << "\n";

					if (operand1 == NULL)
					{
//						errs() << "CastInst op1 is not a variable!\n";
						continue;
					}

					if (operand1->getMetadata("EFFECT") == NULL) // some weird case.. just say that it was changed
					{
						generateMetadata(changedString, NULL, NULL, CI, M);
						continue;
					}
					
					MDNode* operandMetadata = operand1->getMetadata("EFFECT");

					if (operandMetadata->getOperand(1) == NULL) // operand refers to changed variable
					{
						generateMetadata(changedString, NULL, NULL, CI, M);
						continue;
					}
					else
					{
						MDString* prev = (MDString*)(operand1->getMetadata("EFFECT")->getOperand(0));
						Value* variable = operandMetadata->getOperand(1);
						Value* n = operandMetadata->getOperand(2);
						generateMetadata(prev, variable, n, CI, M);
						continue;
					}
				}
                
				if (BinaryOperator *BO = dyn_cast<BinaryOperator>(inst)) 
				{	
//					errs() << "Inspecting BinOp Instruction\n";
					bool isOperand1Constant, isOperand2Constant;
					Value* operand1 = BO->getOperand(0);
					Value* operand2 = BO->getOperand(1);
					Value* variable = NULL;
					Value* constant = NULL;
					Instruction* operand = NULL;
					
					int variablePos, constantPos;

					isOperand1Constant = dyn_cast<ConstantInt>(operand1) || dyn_cast<ConstantFP>(operand1);
					isOperand2Constant = dyn_cast<ConstantInt>(operand2) || dyn_cast<ConstantFP>(operand2);
					
					std::vector<Value*> effect;
					
					// if two operands of binary ops are non-constants, then the value is marked as "changed" for both variables
					if (!isOperand1Constant && !isOperand2Constant)
					{
						generateMetadata(changedString, NULL, NULL, BO, M);
						continue;
					}
					else
					{
						if (!isOperand1Constant)
						{
							variablePos = 0;
							constantPos = 1;
						}
						else
						{
							variablePos = 1;
							constantPos = 0;
						}

						operand = dyn_cast<Instruction>(BO->getOperand(variablePos));
						
						MDNode* operandMetadata = operand->getMetadata("EFFECT");

						if (operandMetadata == NULL) // variable reference is from another block or this is a pointer
						{
							//errs() << "VARIABLE REFERENCE IS FROM ANOTHER BLOCK!!\n";
							generateMetadata(changedString, NULL, NULL, BO, M);
							continue;
						}

						if (operandMetadata->getOperand(1) == NULL) // operand refers to changed variable
						{
							generateMetadata(changedString, NULL, NULL, BO, M);
							continue;
						}
							
						variable = operandMetadata->getOperand(1);
						constant = BO->getOperand(constantPos);								

						MDNode* metadata = operand->getMetadata("EFFECT");
						MDString* mdstr = dyn_cast<MDString>(metadata->getOperand(0));

						Instruction::BinaryOps binOp = BO->getOpcode();
				
						switch (binOp)
						{
							case Instruction::Add:
							{
//								errs() << *BO <<  " BINOP: ADD\n";

								if (ConstantInt* CI = dyn_cast<ConstantInt>(constant))
								{
									int64_t constValue = CI->getSExtValue();
									
									if (!mdstr->getString().equals("CHANGED"))
									{
										if (ConstantInt* ci = dyn_cast<ConstantInt>(metadata->getOperand(2)))
										{
											int64_t constValue2 = ci->getSExtValue();
											int64_t sum = constValue + constValue2;
											ConstantInt* sumValue = ConstantInt::get(Type::getInt64Ty(context), sum);

											if (sum > 0)
											{
												generateMetadata(incrementString, variable, sumValue, BO, M);
												continue;
											}
											else if (sum == 0)
											{
												generateMetadata(unchangedString, variable, sumValue, BO, M);
												continue;
											}
											else
											{
												generateMetadata(decrementString, variable, sumValue, BO, M);
												continue;
											}
										}
									}
									else
									{
										generateMetadata(changedString, NULL, NULL, BO, M);
										continue;
									}
								}
								else
								{
										errs() << "ERROR!!!!!!! UNKNOWN CONSTANT VALUE TYPE FOUND!!!!\n";
										continue;
								}
								break;
							}
							case Instruction::Sub:
							{
//								errs() << *BO <<  " BINOP: SUB\n";

								if (ConstantInt* CI = dyn_cast<ConstantInt>(constant))
								{
									int64_t constValue = -(CI->getSExtValue());
									
									if (!mdstr->getString().equals("CHANGED"))
									{
										if (ConstantInt* ci = dyn_cast<ConstantInt>(metadata->getOperand(2)))
										{
											int64_t constValue2 = ci->getSExtValue();
											int64_t sum = constValue + constValue2;
											ConstantInt* sumValue = ConstantInt::get(Type::getInt64Ty(context), sum);

											if (sum > 0)
											{
												generateMetadata(incrementString, variable, sumValue, BO, M);
												continue;
											}
											else if (sum == 0)
											{
												generateMetadata(unchangedString, variable, sumValue, BO, M);
												continue;
											}
											else
											{
												generateMetadata(decrementString, variable, sumValue, BO, M);
												continue;
											}
										}
									}
									else
									{
										generateMetadata(changedString, NULL, NULL, BO, M);
										continue;
									}									
								}
								else
								{
//										errs() << "ERROR!!!!!!! UNKNOWN CONSTANT VALUE TYPE FOUND!!!!\n";
										continue;
								}
								break;
							}
							default:
							{
								generateMetadata(changedString, NULL, NULL, BO, M);
								continue;
								break;
							}
						}
					}
				}
		}
				
//        errs() << "Exiting basic block\n\n";
        return true;
}

void EffectGenPassMore::generateMetadata(MDString* str, Value* variable, Value* n, Instruction* inst, Module* M)
{
	LLVMContext& context = M->getContext();
	std::vector<Value*> effect;
	effect.push_back(str);
	effect.push_back(variable);
	effect.push_back(n);
	MDNode* effects = MDNode::get(context, effect);
	inst->setMetadata("EFFECT", effects);
}
	/*
void EffectGenPassMore::generateMetadata(MDString* str, Value* variable, Value* n, Instruction* inst, Module* M)
{
	MDNode* prev = inst->getMetadata("EFFECT");

	if (prev == NULL)
	{
		changed = true;
		LLVMContext& context = M->getContext();
		std::vector<Value*> effect;
		effect.push_back(str);
		effect.push_back(variable);
		effect.push_back(n);
		MDNode* effects = MDNode::get(context, effect);
		inst->setMetadata("EFFECT", effects);
		return;
	}

	MDString* prevString = dyn_cast<MDString>(prev->getOperand(0));

	if (prevString->getString().equals(str->getString()))
	{
		if (prevString->getString().equals("CHANGED"))
		{
			return;
		}
		else
		{
			Value* prevVar = prev->getOperand(1);
			ConstantInt* prevN = dyn_cast<ConstantInt>(prev->getOperand(2));
			ConstantInt* currentN = dyn_cast<ConstantInt>(n);
	
			if (prevString->getString().equals(str->getString()) && (prevVar == variable) && (prevN->getSExtValue() == currentN->getSExtValue()))
			{
				return;
			}
		}
	}

	changed = true;
	LLVMContext& context = M->getContext();
	std::vector<Value*> effect;
	effect.push_back(str);
	effect.push_back(variable);
	effect.push_back(n);
	MDNode* effects = MDNode::get(context, effect);
	inst->setMetadata("EFFECT", effects);
}*/
