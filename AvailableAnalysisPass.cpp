#include "AvailableAnalysisPass.h"

char AvailableAnalysisPass::ID = 0;
static RegisterPass<AvailableAnalysisPass> F("available-analysis", "Global Available Bound Checks Analysis", false, false);

/*RangeCheckSet* SetUnion(RangeCheckSet* S1, RangeCheckSet* S2)
{
	RangeCheckSet* unionSet = S1->set_union(S2);
	return unionSet;
}*/

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

bool AvailableAnalysisPass::runOnModule(Module& M)
{
	errs() << "\n#########################################\n";
	errs() << "AVAILABLE ANALYSIS\n";
	errs() << "#########################################\n";
    
	this->module = &M;        
	this->BB_A_OUT = new MapBBToRCS();
	this->I_A_OUT = new MapInstToRCS();
	
	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i)
	{
		Function* func = &(*i);
		runOnFunction(&(*func));
	}	

	return false;
}

bool AvailableAnalysisPass::runOnFunction(Function* F)
{
	this->currentFunction = F;

	if (!F->isDeclaration())
	{
		this->dataFlowAnalysis();
	}
	return true;
}

void AvailableAnalysisPass::createUniverse()
{		
	this->universe = new RangeCheckSet();	
	
	for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
	{
		BasicBlock* BB = &*BBI;
		for(BasicBlock::iterator II = BB->begin(), IE = BB->end(); II != IE; II++)
		{
			Instruction* inst = &*II;	
			if (CallInst *ci = dyn_cast<CallInst> (inst))
			{
				if (ci->getCalledFunction() == NULL)
					continue;

				const StringRef& callFunctionName = ci->getCalledFunction()->getName();
               if (callFunctionName.equals("checkLessThan"))
			   {
					RangeCheckSet* tmpUniverse = universe->set_union(new RangeCheckExpression(ci, this->module));
					delete this->universe;
					this->universe = tmpUniverse;
			   }
			}
		}
	}
	
	//errs() << "Universe: "; this->universe->println();
}

void AvailableAnalysisPass::dataFlowAnalysis()
{
	this->createUniverse();
	
	///initialize all blocks sets
	for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
	{
		///out of the block is universe
		BasicBlock* BB = &*BBI;
		this->BB_A_OUT->insert(PairBBAndRCS(BB, universe));
	}

	int i = 0;
	bool isChanged = true;
//	errs() << "******************* AVAILABLE ANALYSIS *************************\n";
	while (isChanged)
	{
		isChanged = false;
		
		///go throught all of the blocks
//		errs() << "******************* ROUND " << i << "*************************\n";
		for(Function::iterator BBI = this->currentFunction->begin(), BBE = this->currentFunction->end(); BBI != BBE; BBI++)
		{
			BasicBlock* BB = &*BBI;
//			errs() << "Basic Block: " << BB->getName() << "\n";
			ListRCS predRCS;

			///get a list of range check sets
			for(pred_iterator PBBI = pred_begin(BB), PBBE = pred_end(BB); PBBI != PBBE; PBBI++)
			{
				predRCS.push_back((*BB_A_OUT)[(BasicBlock*)(*PBBI)]);
			}

			///calculate the OUT of the block, by intersecting all predecessors IN's
			RangeCheckSet *C_IN = SetsMeet(&predRCS, SetIntersection);
			//errs() << "IN = "; C_IN->println();

			///calculate the IN of the block, running the functions we already created
			RangeCheckSet *C_OUT = this->getAvailOut(BB, C_IN);
			//errs() << "OUT = "; C_OUT->println();

			RangeCheckSet *C_OUT_P = BB_A_OUT->find(BB)->second;
			//errs() << "OUT_PREV = "; C_OUT_P->println();

			BB_A_OUT->erase(BB);

			//compare C_OUT and C_OUT_PREV
			if(!C_OUT_P->equal(C_OUT))
			{
				isChanged = true;	
//				errs() << "changed\n";
			}
			else
			{
//				errs() << "unchanged\n";
			}

			BB_A_OUT->insert(PairBBAndRCS(BB, C_OUT));

//			errs() << "\n";
		}	
//		errs() << "***************************************************\n";
		i++;
	}
}
RangeCheckSet *AvailableAnalysisPass::getAvailOut(BasicBlock *BB, RangeCheckSet *cInOfBlock)
{
	std::map<Instruction*, bool> stored;
    RangeCheckSet* currentRCS = cInOfBlock;
	llvm::BasicBlock::InstListType& instList = BB->getInstList();
	for(BasicBlock::InstListType::iterator II = instList.begin(), EI = instList.end(); II != EI; II++)
	{
		if (StoreInst* sti = dyn_cast<StoreInst>(&(*II)))
		{
			if (dyn_cast<AllocaInst>(sti->getOperand(1)))
				stored[&(*II)] = true;
		}
	
#ifdef __MORE__
		EffectGenMore::generateEffectMore(&(*II), BB, this->module);
#else
		EffectGen::generateEffect(&(*II), this->module);
#endif
	
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

			//INSERT AVAILABLE INFORMATION FOR THIS CALL INSTRUCTION!
			I_A_OUT->erase(callInst);
			this->I_A_OUT->insert(PairIAndRCS(callInst, currentRCS));
					
			RangeCheckExpression* rce = new RangeCheckExpression(callInst, this->module);
			//errs() << "\t\tRCS Generated: "; rce->println();
			RangeCheckSet* tmp = currentRCS->set_union(rce);
			//errs() << "\t\tOUT: "; tmp->println();
			currentRCS = tmp;
        }
		else if (false)
		{	
			// NEED TO KILL VARIABLE OF WHICH ITS ADDRESS GETS LOADED
		}
        else if(StoreInst* storeInst = dyn_cast<StoreInst>(&*II))
		{
			//errs() << "Store Instruction: " << *storeInst << "\n";
			currentRCS->kill_forward(storeInst, this->module);
        }
   }
	
/*	
	for (std::map<Instruction*, bool>::iterator i = stored.begin(), e = stored.end(); i != e; i++)
	{
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
