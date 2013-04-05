// Generated by llvm2cpp - DO NOT MODIFY!

#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Constants.h>
#include <llvm/GlobalVariable.h>
#include <llvm/Function.h>
#include <llvm/CallingConv.h>
#include <llvm/BasicBlock.h>
#include <llvm/Instructions.h>
#include <llvm/InlineAsm.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <algorithm>
using namespace llvm;

Module* makeLLVMModule();

int main(int argc, char**argv) {
  Module* Mod = makeLLVMModule();
  verifyModule(*Mod, PrintMessageAction);
  PassManager PM;
  PM.add(createPrintModulePass(&outs()));
  PM.run(*Mod);
  return 0;
}


Module* makeLLVMModule() {
 // Module Construction
 Module* mod = new Module("test.ll", getGlobalContext());
 mod->setDataLayout("e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128");
 mod->setTargetTriple("x86_64-apple-macosx10.8.0");
 
 // Type Definitions
 std::vector<Type*>FuncTy_0_args;
 FuncTy_0_args.push_back(IntegerType::get(mod->getContext(), 32));
 PointerType* PointerTy_2 = PointerType::get(IntegerType::get(mod->getContext(), 8), 0);
 
 PointerType* PointerTy_1 = PointerType::get(PointerTy_2, 0);
 
 FuncTy_0_args.push_back(PointerTy_1);
 FunctionType* FuncTy_0 = FunctionType::get(
  /*Result=*/IntegerType::get(mod->getContext(), 32),
  /*Params=*/FuncTy_0_args,
  /*isVarArg=*/false);
 
 PointerType* PointerTy_3 = PointerType::get(IntegerType::get(mod->getContext(), 32), 0);
 
 PointerType* PointerTy_4 = PointerType::get(PointerTy_1, 0);
 
 std::vector<Type*>FuncTy_6_args;
 FunctionType* FuncTy_6 = FunctionType::get(
  /*Result=*/PointerTy_2,
  /*Params=*/FuncTy_6_args,
  /*isVarArg=*/false);
 
 PointerType* PointerTy_5 = PointerType::get(FuncTy_6, 0);
 
 std::vector<Type*>FuncTy_8_args;
 FuncTy_8_args.push_back(PointerTy_2);
 FunctionType* FuncTy_8 = FunctionType::get(
  /*Result=*/Type::getVoidTy(mod->getContext()),
  /*Params=*/FuncTy_8_args,
  /*isVarArg=*/false);
 
 PointerType* PointerTy_7 = PointerType::get(FuncTy_8, 0);
 
 
 // Function Declarations
 
 Function* func_main = mod->getFunction("main");
 if (!func_main) {
 func_main = Function::Create(
  /*Type=*/FuncTy_0,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"main", mod); 
 func_main->setCallingConv(CallingConv::C);
 }
 AttrListPtr func_main_PAL;
 {
  SmallVector<AttributeWithIndex, 4> Attrs;
  AttributeWithIndex PAWI;
  PAWI.Index = 4294967295U;
 {
    AttrBuilder B;
    B.addAttribute(Attributes::NoUnwind);
    B.addAttribute(Attributes::StackProtect);
    B.addAttribute(Attributes::UWTable);
    PAWI.Attrs = Attributes::get(mod->getContext(), B);
 }
  Attrs.push_back(PAWI);
  func_main_PAL = AttrListPtr::get(mod->getContext(), Attrs);
  
 }
 func_main->setAttributes(func_main_PAL);
 
 Function* func_llvm_stacksave = mod->getFunction("llvm.stacksave");
 if (!func_llvm_stacksave) {
 func_llvm_stacksave = Function::Create(
  /*Type=*/FuncTy_6,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"llvm.stacksave", mod); // (external, no body)
 func_llvm_stacksave->setCallingConv(CallingConv::C);
 }
 AttrListPtr func_llvm_stacksave_PAL;
 {
  SmallVector<AttributeWithIndex, 4> Attrs;
  AttributeWithIndex PAWI;
  PAWI.Index = 4294967295U;
 {
    AttrBuilder B;
    B.addAttribute(Attributes::NoUnwind);
    PAWI.Attrs = Attributes::get(mod->getContext(), B);
 }
  Attrs.push_back(PAWI);
  func_llvm_stacksave_PAL = AttrListPtr::get(mod->getContext(), Attrs);
  
 }
 func_llvm_stacksave->setAttributes(func_llvm_stacksave_PAL);
 
 Function* func_llvm_stackrestore = mod->getFunction("llvm.stackrestore");
 if (!func_llvm_stackrestore) {
 func_llvm_stackrestore = Function::Create(
  /*Type=*/FuncTy_8,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"llvm.stackrestore", mod); // (external, no body)
 func_llvm_stackrestore->setCallingConv(CallingConv::C);
 }
 AttrListPtr func_llvm_stackrestore_PAL;
 {
  SmallVector<AttributeWithIndex, 4> Attrs;
  AttributeWithIndex PAWI;
  PAWI.Index = 4294967295U;
 {
    AttrBuilder B;
    B.addAttribute(Attributes::NoUnwind);
    PAWI.Attrs = Attributes::get(mod->getContext(), B);
 }
  Attrs.push_back(PAWI);
  func_llvm_stackrestore_PAL = AttrListPtr::get(mod->getContext(), Attrs);
  
 }
 func_llvm_stackrestore->setAttributes(func_llvm_stackrestore_PAL);
 
 // Global Variable Declarations

 
 // Constant Definitions
 ConstantInt* const_int32_9 = ConstantInt::get(mod->getContext(), APInt(32, StringRef("1"), 10));
 ConstantInt* const_int32_10 = ConstantInt::get(mod->getContext(), APInt(32, StringRef("4"), 10));
 ConstantInt* const_int32_11 = ConstantInt::get(mod->getContext(), APInt(32, StringRef("2"), 10));
 ConstantInt* const_int32_12 = ConstantInt::get(mod->getContext(), APInt(32, StringRef("3"), 10));
 ConstantInt* const_int32_13 = ConstantInt::get(mod->getContext(), APInt(32, StringRef("0"), 10));
 
 // Global Variable Definitions
 
 // Function Definitions
 
 // Function: main (func_main)
 {
  Function::arg_iterator args = func_main->arg_begin();
  Value* int32_argc = args++;
  int32_argc->setName("argc");
  Value* ptr_argv = args++;
  ptr_argv->setName("argv");
  
  BasicBlock* label_entry = BasicBlock::Create(mod->getContext(), "entry",func_main,0);
  
  // Block entry (label_entry)
  AllocaInst* ptr_argc_addr = new AllocaInst(IntegerType::get(mod->getContext(), 32), "argc.addr", label_entry);
  ptr_argc_addr->setAlignment(4);
  AllocaInst* ptr_argv_addr = new AllocaInst(PointerTy_1, "argv.addr", label_entry);
  ptr_argv_addr->setAlignment(8);
  AllocaInst* ptr_n = new AllocaInst(IntegerType::get(mod->getContext(), 32), "n", label_entry);
  ptr_n->setAlignment(4);
  AllocaInst* ptr_saved_stack = new AllocaInst(PointerTy_2, "saved_stack", label_entry);
  StoreInst* void_14 = new StoreInst(int32_argc, ptr_argc_addr, false, label_entry);
  void_14->setAlignment(4);
  StoreInst* void_15 = new StoreInst(ptr_argv, ptr_argv_addr, false, label_entry);
  void_15->setAlignment(8);
  StoreInst* void_16 = new StoreInst(const_int32_10, ptr_n, false, label_entry);
  void_16->setAlignment(4);
  LoadInst* int32_17 = new LoadInst(ptr_n, "", false, label_entry);
  int32_17->setAlignment(4);
  CastInst* int64_18 = new ZExtInst(int32_17, IntegerType::get(mod->getContext(), 64), "", label_entry);
  CallInst* ptr_19 = CallInst::Create(func_llvm_stacksave, "", label_entry);
  ptr_19->setCallingConv(CallingConv::C);
  ptr_19->setTailCall(false);
  AttrListPtr ptr_19_PAL;
  ptr_19->setAttributes(ptr_19_PAL);
  
  StoreInst* void_20 = new StoreInst(ptr_19, ptr_saved_stack, false, label_entry);
  AllocaInst* ptr_vla = new AllocaInst(IntegerType::get(mod->getContext(), 32), int64_18, "vla", label_entry);
  ptr_vla->setAlignment(16);
  LoadInst* int32_21 = new LoadInst(ptr_n, "", false, label_entry);
  int32_21->setAlignment(4);
  BinaryOperator* int32_add = BinaryOperator::Create(Instruction::Add, int32_21, const_int32_9, "add", label_entry);
  StoreInst* void_22 = new StoreInst(int32_add, ptr_n, false, label_entry);
  void_22->setAlignment(4);
  LoadInst* int32_23 = new LoadInst(ptr_n, "", false, label_entry);
  int32_23->setAlignment(4);
  BinaryOperator* int32_add1 = BinaryOperator::Create(Instruction::Add, int32_23, const_int32_11, "add1", label_entry);
  StoreInst* void_24 = new StoreInst(int32_add1, ptr_n, false, label_entry);
  void_24->setAlignment(4);
  LoadInst* int32_25 = new LoadInst(ptr_n, "", false, label_entry);
  int32_25->setAlignment(4);
  CastInst* int64_idxprom = new SExtInst(int32_25, IntegerType::get(mod->getContext(), 64), "idxprom", label_entry);
  GetElementPtrInst* ptr_arrayidx = GetElementPtrInst::Create(ptr_vla, int64_idxprom, "arrayidx", label_entry);
  StoreInst* void_26 = new StoreInst(const_int32_12, ptr_arrayidx, false, label_entry);
  void_26->setAlignment(4);
  StoreInst* void_27 = new StoreInst(const_int32_12, ptr_n, false, label_entry);
  void_27->setAlignment(4);
  LoadInst* int32_28 = new LoadInst(ptr_n, "", false, label_entry);
  int32_28->setAlignment(4);
  CastInst* int64_idxprom2 = new SExtInst(int32_28, IntegerType::get(mod->getContext(), 64), "idxprom2", label_entry);
  GetElementPtrInst* ptr_arrayidx3 = GetElementPtrInst::Create(ptr_vla, int64_idxprom2, "arrayidx3", label_entry);
  StoreInst* void_29 = new StoreInst(const_int32_12, ptr_arrayidx3, false, label_entry);
  void_29->setAlignment(4);
  LoadInst* ptr_30 = new LoadInst(ptr_saved_stack, "", false, label_entry);
  CallInst* void_31 = CallInst::Create(func_llvm_stackrestore, ptr_30, "", label_entry);
  void_31->setCallingConv(CallingConv::C);
  void_31->setTailCall(false);
  AttrListPtr void_31_PAL;
  void_31->setAttributes(void_31_PAL);
  
  ReturnInst::Create(mod->getContext(), const_int32_13, label_entry);
  
 }
 
 return mod;
}