#!/bin/bash

TEST_FILE=test.c
TEST_NAME=""
OPT_PASSES=""
LLVM_LIBRARY=../../Release+Asserts/
MODULE_LIB=`ls ${LLVM_LIBRARY}lib/llvm-array-check-pass*`
OPT="${LLVM_LIBRARY}bin/opt"

#other optimizations can added by opt as argument
#all optimization can be obtained by -help


#DIFFERENT WAYS TO RUN OPT UNDER
case "$1" in
	'-checks-only')	
		TEST_FILE=test.c
		OPT_PASSES="-array-check"
		;;
	'-local-opts')
		TEST_FILE=test.c
		OPT_PASSES="-array-check -local-opts"
		;;
	'-global-opts-only')
		TEST_FILE=globalOptTest.c
		OPT_PASSES="-array-check -global-opts"
		;;
	'-global-opts')
		TEST_FILE=globalOptTest.c
		OPT_PASSES="-array-check -local-opts -global-opts"
		;;
	*)
		echo "invalid argument,"
		echo "Usage: runPass	[ -checks-only | -local-opts | -global-opts-only | -global-opts ]"
		exit 1
		;;
esac

#remove suffix .c
TEST_NAME=${TEST_FILE%.c}

#compiles the test file, and the check library
clang -emit-llvm -S -o $TEST_NAME.ll $TEST_NAME.c
clang++ -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -emit-llvm -S -o LibArrayCheck.ll LibArrayCheck.cpp

#run opt, with specified passes
./$OPT -load $MODULE_LIB $OPT_PASSES -debug-pass=Structure $2 -S -o $TEST_NAME.mod.ll < $TEST_NAME.ll > /dev/null


#the test code and library code are linked into executable
clang -lstdc++ -o $TEST_NAME.out $TEST_NAME.mod.ll LibArrayCheck.ll

#clean up al intermediate files, unless specified
#if [[ $1 != "-i" ]] 
#then
#	rm -rf *.bc *.ll *.o
#fi
	
