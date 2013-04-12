#include "VeryBusyAnalysisPass.h"

char VeryBusyAnalysisPass::ID = 0;
static RegisterPass<VeryBusyAnalysisPass> E("very-busy-analysis", "Global Very Busy Array Bound Checks Analysis", false, false);

static RangeCheckSet* SetIntersection(RangeCheckSet* S1, RangeCheckSet* S2)
{
	RangeCheckSet* intersectSet = S1->set_intersect(S2);
	return intersectSet;
}

static RangeCheckSet* SetsMeet(ListRCS* sets, RangeCheckSet*(*meet)(RangeCheckSet*, RangeCheckSet*))
{
	if (sets->size() == 0)
		return new RangeCheckSet();
	
	RangeCheckSet* lastSet = *(sets->begin());

	for(std::list<RangeCheckSet*>::iterator II = sets->begin(), IE = sets->end(); II != IE; II++)
	{
		RangeCheckSet* currentSet = meet(lastSet, *II);
		lastSet = currentSet;
	}
	return lastSet;
}

bool VeryBusyAnalysisPass::runOnModule(Module& M)
{
	errs() << "\n#########################################\n";
	errs() << "VERY BUSY ANALYSIS\n";
	errs() << "#########################################\n";
    
     this->module = &M;
	 this->BB_VB_IN = new MapBBToRCS();
	 this->I_VB_IN = new MapInstToRCS();

	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i)
	{
		Function* func = &(*i);
		runOnFunction(&(*func));
	}

	 return false;
}

bool VeryBusyAnalysisPass::runOnFunction(Function* F)
{
	this->currentFunction = F;

	if (!F->isDeclaration())
	{
		this->dataFlowAnalysis();
	}
	return false;
}


void VeryBusyAnalysisPass::createUniverse()
{
	this->universe = new RangeCheckSet();

	for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
	{
		BasicBlock* BB = &*BBI;
		
		for(BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; II++)
		{
			Instruction* inst = &*II;	

			if(CallInst *ci = dyn_cast<CallInst> (inst))
			{
				if (ci->getCalledFunction() == NULL)
					continue;

				const StringRef& callFunctionName = ci->getCalledFunction()->getName();

				if(callFunctionName.equals("checkLessThan"))
				{
					RangeCheckExpression* newExpr = new RangeCheckExpression(ci, this->module);
					//errs() << "being unioned: "; newExpr->println();
					RangeCheckSet* tmpUniverse = universe->set_union(newExpr);
					//errs() << "union created: "; tmpUniverse->println();
					delete this->universe;
					this->universe = tmpUniverse;
				}
			}
		}
	}

	//errs() << "Universe: "; this->universe->println();
}

void VeryBusyAnalysisPass::dataFlowAnalysis()
{
	this->createUniverse();
	
	///initialize all blocks sets
	for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
	{
		///in of the block is universe
		BasicBlock* BB = &*BBI;
		this->BB_VB_IN->insert(PairBBAndRCS(BB, universe));
	}

	int i = 0;
	bool isChanged = true;
	
	while(isChanged)
	{
		isChanged = false;

		///go throught all of the blocks
//		errs() << "******************* ROUND " << i << "*************************\n";

		for (Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
		{
			BasicBlock* BB = &*BBI;
//			errs() << "Basic Block: " << BB->getName() << "\n";
			
			ListRCS succsRCS;
			
			///get a list of range check sets
			for(succ_iterator SBBI = succ_begin(BB), SBBE = succ_end(BB); SBBI != SBBE; SBBI++){
				succsRCS.push_back((*BB_VB_IN)[*SBBI]);
			}

			///calculate the OUT of the block, by intersecting all successors IN's
			RangeCheckSet *C_OUT = SetsMeet(&succsRCS, SetIntersection);
//			errs() << "OUT = "; C_OUT->println();

			///calculate the IN of the block, running the functions we already created
			RangeCheckSet *C_IN = this->getVBIn(BB, C_OUT);
//			errs() << "IN = "; C_IN->println();
			
			RangeCheckSet *C_IN_P = BB_VB_IN->find(BB)->second;
//			errs() << "IN_PREV = "; C_IN_P->println();
			
			BB_VB_IN->erase(BB);
			
			// compare C_IN with our previous C_IN
			if(!C_IN_P->equal(C_IN))
			{
				isChanged = true;	
//				errs() << "changed\n";
			}
			else
			{
//				errs() << "unchanged\n";
			}

			BB_VB_IN->insert(PairBBAndRCS(BB, C_IN));
			
//			errs() << "\n";
		}
//		errs() << "***************************************************\n";
		i++;
	}
}

RangeCheckSet *VeryBusyAnalysisPass::getVBIn(BasicBlock *BB, RangeCheckSet *cOutOfBlock)
{
	std::map<Instruction*, bool> stored;
	RangeCheckSet* currentRCS = cOutOfBlock;
	llvm::BasicBlock::InstListType& instList = BB->getInstList();
	for(BasicBlock::InstListType::reverse_iterator II = instList.rbegin(), EI = instList.rend(); II != EI; II++)
	{
		for (BasicBlock::InstListType::iterator i = instList.begin(), e = &(*II); i != e; i++)
		{
			if (StoreInst* sti = dyn_cast<StoreInst>(&(*i)))
			{
				if (dyn_cast<AllocaInst>(sti->getOperand(1)))
					stored[&(*i)] = true;
			}

#ifdef __MORE__
			EffectGenMore::generateEffectMore(&(*i), BB, this->module);
#else
			EffectGen::generateEffect(&(*i), this->module);
#endif
		}
		
		if(CallInst* callInst = dyn_cast<CallInst>(&*II))
		{
			if (callInst->getCalledFunction() == NULL)
				continue;
			
			const StringRef& callFunctionName = callInst->getCalledFunction()->getName();
			
			if(!callFunctionName.equals("checkLessThan"))
			{
				continue;
			}
			
			//errs() << "Call Instruction: " << *callInst << "\n";
			
			I_VB_IN->erase(callInst);
			this->I_VB_IN->insert(PairIAndRCS(callInst, currentRCS));

			RangeCheckExpression* rce = new RangeCheckExpression(callInst, this->module);
			//errs() << "\t\tRCS Generated: "; rce->println();
			//errs() << "\t\tCurrent RCS "; currentRCS->println();
			RangeCheckSet* tmp = currentRCS->set_union(rce);
			//errs() << "\t\tIN: "; tmp->println();
			currentRCS = tmp;
		}
		else if (false)
		{	
			// NEED TO KILL VARIABLE OF WHICH ITS ADDRESS GETS LOADED
		}
		else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II))
		{
			//errs() << "Store Instruction: " << *storeInst << "\n";
			currentRCS->kill_backward(storeInst, this->module);
		}
	}

	/*
	for (std::map<Instruction*, bool>::iterator i = stored.begin(), e = stored.end(); i != e; i++)
	{
		errs() << "store inst " << *(i->first) << "\n";
		LLVMContext& context = this->module->getContext();
		MDString* unchangedString = MDString::get(context, "UNCHANGED");
		Value* var = i->first->getOperand(1);
		ConstantInt* zero = ConstantInt::get(Type::getInt64Ty(context), 0);
	
		std::vector<Value*> effect;
		effect.push_back(unchangedString);
		effect.push_back(var);
		effect.push_back(zero);
		MDNode* effects = MDNode::get(context, effect);

		dyn_cast<Instruction>(var)->setMetadata("EFFECT", effects);
	}*/


	return currentRCS;
}
