#include "LocalOptimizationsOnArrayChecks.h"
#include "llvm/GlobalVariable.h"
#include "llvm/GlobalValue.h"
#include "stdlib.h"
#include <set>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include "ArrayBoundsCheckPass.h"

using namespace llvm;

char LocalOptimizationsOnArrayChecks::ID = 0;
static RegisterPass<LocalOptimizationsOnArrayChecks> C("local-opts", "Local optimizations on array checks performed", false, false);


bool LocalOptimizationsOnArrayChecks::doInitialization(Module& M)
{
	this->M = &M;
	// stub function. do not delete. keeps the compiler warnings and errors at bay
    return false;
}

bool LocalOptimizationsOnArrayChecks::doInitialization(Function& F)
{
        errs() << "\n#########################################\n";
        errs() << "Beginning LocalOptimizationsOnArrayChecks\n";
        errs() << "#########################################\n";
        return false;
}

bool LocalOptimizationsOnArrayChecks::runOnBasicBlock(BasicBlock& BB)
{
		LLVMContext& context = this->M->getContext();
		MDString* useString = MDString::get(context, "USE"); // should be replaced by one of below
		MDString* unchangedString = MDString::get(context, "UNCHANGED");
		MDString* incrementString = MDString::get(context, "INCREMENT");
		MDString* decrementString = MDString::get(context, "DECREMENT");
		MDString* multiplyString = MDString::get(context, "MULTIPLY");
		MDString* divGTString = MDString::get(context, "DIVGTONE");
		MDString* divLessString = MDString::get(context, "DIVLESSONE");
		MDString* changedString = MDString::get(context, "CHANGED");
        
		errs() << "Inspecting a new basic block...\n";
        // hold an array of all of the call instructions in that basic block
        // every time we see a call instruction, loop over all the ones seen so far
        // if the exact check has already been made,
        // remove it

        /*
         * identical checks optimization:
         *  if C comes before C', and they are identical checks,
         *  then as long as the variables used in the check are not redefined between them,
         *  then we can eliminate C'
         */

        // iterate over all instructions in a basic block
        for (BasicBlock::iterator i = BB.begin(), e = BB.end(); i != e; ++i) 
		{
                Instruction *inst = &*i;

                if (LoadInst *LI = dyn_cast<LoadInst>(inst)) 
				{
					if (AllocaInst* alloca = dyn_cast<AllocaInst>(LI->getOperand(0)))
					{
						std::vector<Value*> effect;

						effect.push_back(unchangedString);
						effect.push_back(LI->getOperand(0));

						MDNode* effects = MDNode::get(context, effect);
						LI->setMetadata("EFFECT", effects);
					}
				}
                
				if (BinaryOperator *BO = dyn_cast<BinaryOperator>(inst)) 
				{					
					bool isOperand1Constant, isOperand2Constant;
					Value* operand1 = BO->getOperand(0);
					Value* operand2 = BO->getOperand(1);
					Value* variable = NULL;
					Value* constant = NULL;

					isOperand1Constant = dyn_cast<ConstantInt>(operand1) || dyn_cast<ConstantFP>(operand1);
					isOperand2Constant = dyn_cast<ConstantInt>(operand2) || dyn_cast<ConstantFP>(operand2);
					
					std::vector<Value*> effect;
					
					if (!isOperand1Constant && !isOperand2Constant)
					{
						effect.push_back(changedString);
						effect.push_back(NULL);
						MDNode* effects = MDNode::get(context, effect);
						BO->setMetadata("EFFECT", effects);
						continue;
					}
					else
					{
						if (!isOperand1Constant)
						{
							variable = ((Instruction*)BO->getOperand(0))->getMetadata("EFFECT")->getOperand(1);
							constant = operand2;
						}
						else
						{
							variable = ((Instruction*)BO->getOperand(1))->getMetadata("EFFECT")->getOperand(1);
							constant = operand1;
						}
												
						Instruction::BinaryOps binOp = BO->getOpcode();
				
						switch (binOp)
						{
							case Instruction::Add:
							case Instruction::FAdd:
								errs() << *BO <<  " BINOP: ADD\n";
							
								if (ConstantInt* CI = dyn_cast<ConstantInt>(constant))
								{
									int64_t constValue = CI->getSExtValue();
									if (constValue > 0)
									{
										effect.push_back(incrementString);
										effect.push_back(variable);
										MDNode* effects = MDNode::get(context, effect);
										BO->setMetadata("EFFECT", effects);
									}
									else if (constValue == 0)
									{
										effect.push_back(unchangedString);
										effect.push_back(variable);
										MDNode* effects = MDNode::get(context, effect);
										BO->setMetadata("EFFECT", effects);
									}
									else
									{
										effect.push_back(changedString);
										effect.push_back(NULL);
										MDNode* effects = MDNode::get(context, effect);
										BO->setMetadata("EFFECT", effects);
									}
								}
								else if (ConstantFP* FP = dyn_cast<ConstantFP>(constant))
								{
									double constValue = (FP->getValueAPF()).convertToDouble();
									
									if (constValue > 0)
									{
										effect.push_back(incrementString);
										effect.push_back(variable);
										MDNode* effects = MDNode::get(context, effect);
										BO->setMetadata("EFFECT", effects);
									}
									else if (constValue == 0)
									{
										effect.push_back(unchangedString);
										effect.push_back(variable);
										MDNode* effects = MDNode::get(context, effect);
										BO->setMetadata("EFFECT", effects);
									}
									else
									{
										effect.push_back(changedString);
										effect.push_back(NULL);
										MDNode* effects = MDNode::get(context, effect);
										BO->setMetadata("EFFECT", effects);
									}
								}
								else
								{
										errs() << "ERROR!!!!!!! UNKNOWN CONSTANT VALUE TYPE FOUND!!!!\n";
								}
								break;
							case Instruction::Sub:
							case Instruction::FSub:
								errs() << *BO <<  " BINOP: SUB\n";
								break;
							case Instruction::Mul:
							case Instruction::FMul:
								errs() << *BO <<  " BINOP: MUL\n";
								break;
							case Instruction::UDiv:
							case Instruction::SDiv:
							case Instruction::FDiv:
								errs() << *BO <<  " BINOP: DIV\n";
								break;
							default:
								errs() << *BO <<  " BINOP: OTHER\n";
								break;
					}

				}
				}
        }

        errs() << "Exiting basic block\n\n";
        return false;
}


