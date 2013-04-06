#include "EffectGenPass.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "llvm/Analysis/Dominators.h"
#include "stdlib.h"
#include "ArrayBoundsCheckPass.h"
#include "RangeCheckExpression.h"
#include "RangeCheckSet.h"
#include <vector>

using namespace llvm;

char EffectGenPass::ID = 0;
static RegisterPass<EffectGenPass> C("effect-gen", "Effect Generation", false, false);

bool EffectGenPass::runOnModule(Module& M)
{
	this->M = &M;

	errs() << "\n#########################################\n";
	errs() << "Generating Effects\n";
	errs() << "#########################################\n";

	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i)
	{
		Function* func = &(*i);
		runOnFunction(&(*func));
	}
	
	errs() << "\n#########################################\n";
	errs() << "DONE\n";
	errs() << "#########################################\n";

	return false;
}

bool EffectGenPass::runOnFunction(Function* F)
{
	for (Function::iterator i = F->begin(), e = F->end(); i != e; ++i)
	{
		runOnBasicBlock(i);
	}
	return true;
}

bool EffectGenPass::runOnBasicBlock(BasicBlock* BB)
{
	errs() << "Entering basic block\n";

		LLVMContext& context = this->M->getContext();
		ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(context), 0);
		
		MDString* unchangedString = MDString::get(context, "UNCHANGED");
		MDString* incrementString = MDString::get(context, "INCREMENT");
		MDString* decrementString = MDString::get(context, "DECREMENT");
		MDString* changedString = MDString::get(context, "CHANGED");
        
		for (BasicBlock::iterator i = BB->begin(), e = BB->end(); i != e; ++i) 
		{
				Instruction *inst = &*i;

				errs() << "inst: " << *inst << "\n";
				
				if (LoadInst *LI = dyn_cast<LoadInst>(inst)) 
				{
					if (dyn_cast<AllocaInst>(LI->getOperand(0)) || dyn_cast<GlobalVariable>(LI->getOperand(0)))
					{
						generateMetadata(unchangedString, LI->getOperand(0), zero, LI, M);
						continue;
					}
				}
				
				if (CastInst* CI = dyn_cast<CastInst>(inst)) 
				{
					Instruction* operand1 = dyn_cast<Instruction>(CI->getOperand(0));

					errs() << "operand: " << *operand1 << "\n";

					if (operand1 == NULL)
					{
						errs() << "ERROR!! CastInst op1 is not a variable!\n";
					}

					if (!(operand1->hasMetadata())) // some weird case.. just say that it was changed
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
					
					// if two operands of binary ops are non-constants, then the value is changed for both variables
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

						if (operandMetadata == NULL) // variable reference is from another block
						{
							errs() << "VARIABLE REFERENCE IS FROM ANOTHER BLOCK!!\n";
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
							case Instruction::FAdd:
							{
								errs() << *BO <<  " BINOP: ADD\n";

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
										else if (ConstantFP* cf = dyn_cast<ConstantFP>(metadata->getOperand(2)))
										{
											double constValue2 = (cf->getValueAPF()).convertToDouble();
											double sum = constValue + constValue2;
											ConstantFP* sumValue = dyn_cast<ConstantFP>(ConstantFP::get(Type::getDoubleTy(context), sum));
											
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
										else
										{
											generateMetadata(changedString, NULL, NULL, BO, M);
											continue;
										}
									}
								}
								else if (ConstantFP* FP = dyn_cast<ConstantFP>(constant))
								{
									double constValue = (FP->getValueAPF()).convertToDouble();
									
									if (!mdstr->getString().equals("CHANGED"))
									{
										if (ConstantInt* ci = dyn_cast<ConstantInt>(metadata->getOperand(2)))
										{
											int64_t constValue2 = ci->getSExtValue();
											double sum = constValue + constValue2;
											ConstantFP* sumValue = dyn_cast<ConstantFP>(ConstantFP::get(Type::getDoubleTy(context), sum));

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
										else if (ConstantFP* cf = dyn_cast<ConstantFP>(metadata->getOperand(2)))
										{
											double constValue2 = (cf->getValueAPF()).convertToDouble();
											double sum = constValue + constValue2;
											ConstantFP* sumValue = dyn_cast<ConstantFP>(ConstantFP::get(Type::getDoubleTy(context), sum));
											
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
							case Instruction::FSub:
							{
								errs() << *BO <<  " BINOP: SUB\n";

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
										else if (ConstantFP* cf = dyn_cast<ConstantFP>(metadata->getOperand(2)))
										{
											double constValue2 = (cf->getValueAPF()).convertToDouble();
											double sum = constValue + constValue2;
											ConstantFP* sumValue = dyn_cast<ConstantFP>(ConstantFP::get(Type::getDoubleTy(context), sum));
											
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
										else
										{
											generateMetadata(changedString, NULL, NULL, BO, M);
											continue;
										}
									}
								}
								else if (ConstantFP* FP = dyn_cast<ConstantFP>(constant))
								{
									double constValue = -((FP->getValueAPF()).convertToDouble());
									
									if (!mdstr->getString().equals("CHANGED"))
									{
										if (ConstantInt* ci = dyn_cast<ConstantInt>(metadata->getOperand(2)))
										{
											int64_t constValue2 = ci->getSExtValue();
											double sum = constValue + constValue2;
											ConstantFP* sumValue = dyn_cast<ConstantFP>(ConstantFP::get(Type::getDoubleTy(context), sum));

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
										else if (ConstantFP* cf = dyn_cast<ConstantFP>(metadata->getOperand(2)))
										{
											double constValue2 = (cf->getValueAPF()).convertToDouble();
											double sum = constValue + constValue2;
											ConstantFP* sumValue = dyn_cast<ConstantFP>(ConstantFP::get(Type::getDoubleTy(context), sum));
											
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
										else
										{
											generateMetadata(changedString, NULL, NULL, BO, M);
											continue;
										}
									}
								}
								else
								{
										errs() << "ERROR!!!!!!! UNKNOWN CONSTANT VALUE TYPE FOUND!!!!\n";
										continue;
								}
								break;
							}
							case Instruction::Mul:
							case Instruction::FMul:
							{
								errs() << *BO <<  " BINOP: MUL\n";
								generateMetadata(changedString, NULL, NULL, BO, M);
								continue;
								break;
							}
							case Instruction::UDiv:
							case Instruction::SDiv:
							case Instruction::FDiv:
							{
								errs() << *BO <<  " BINOP: DIV\n";
								generateMetadata(changedString, NULL, NULL, BO, M);
								continue;
								break;
							}
							default:
							{
								errs() << *BO <<  " BINOP: OTHER\n";
								generateMetadata(changedString, NULL, NULL, BO, M);
								continue;
								break;
							}
						}
					}
				}
				}
		}

        errs() << "Exiting basic block\n\n";
        return true;
}

void EffectGenPass::generateMetadata(MDString* str, Value* variable, Value* n, Instruction* inst, Module* M)
{
	LLVMContext& context = M->getContext();
	std::vector<Value*> effect;
	effect.push_back(str);
	effect.push_back(variable);
	effect.push_back(n);
	MDNode* effects = MDNode::get(context, effect);
	inst->setMetadata("EFFECT", effects);
}
