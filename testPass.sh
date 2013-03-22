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

clang -emit-llvm -c -o  $TEST_NAME.bc $TEST_NAME.c
#array check pass is run on test code after being compiled
opt -load "$LLVM_LIBRARY"lib/llvm-array-check-pass.so -array-check --debug-pass=Structure -o $TEST_NAME.mod.bc < $TEST_NAME.bc > /dev/null
#compile library that will be linked to actual test code
clang++ -emit-llvm -c -o  LibArrayCheck.bc LibArrayCheck.cpp
#the test code and library code are linked into executable
#the stdc++ library is needed by LibArrayCheck
clang -lstdc++ -o $TEST_NAME $TEST_NAME.mod.bc LibArrayCheck.bc
#clean up al intermediate files, unless specified
if [[ $1 != "-i" ]] 
then
	rm -rf *.bc *.ll *.o
fi
	
