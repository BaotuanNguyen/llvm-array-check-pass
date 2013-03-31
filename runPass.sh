#!/bin/bash 

TEST_FILE=test.c
TEST_NAME=${TEST_FILE%.c}
LLVM_LIBRARY=../../Release+Asserts/

if [[ $1 == "--help" ]]
then
	echo "Usage: testPass [--help|-i]";
	echo "--help 		show options available to testPass";
	echo "-i		don't delete intermediate files";
	exit 0;
fi

#other optimizations can added by opt as argument
#all optimization can be obtained by -help

clang -emit-llvm -S -o $TEST_NAME.ll $TEST_NAME.c
clang++ -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -emit-llvm -S -o LibArrayCheck.ll LibArrayCheck.cpp

#array check pass is run on test code after being compiled
opt -load "$LLVM_LIBRARY"lib/llvm-array-check-pass.so -array-check -local-opts -debug-pass=Structure -S -o $TEST_NAME.mod.ll < $TEST_NAME.ll > /dev/null


#the test code and library code are linked into executable
clang -lstdc++ -o $TEST_NAME.out $TEST_NAME.mod.ll LibArrayCheck.ll

#clean up al intermediate files, unless specified
#if [[ $1 != "-i" ]] 
#then
#	rm -rf *.bc *.ll *.o
#fi
	
