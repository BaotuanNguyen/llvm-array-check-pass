#!/bin/bash

TEST_FILE=$2
OPT_PASSES=""
LLVM_LIBRARY=../../Release+Asserts/
MODULE_LIB=`ls ${LLVM_LIBRARY}lib/llvm-array-check-pass*`
OPT="${LLVM_LIBRARY}bin/opt"

#other optimizations can added by opt as argument
#all optimization can be obtained by -help

#DIFFERENT WAYS TO RUN OPT UNDER
case "$1" in
	'-array-check')	
		OPT_PASSES="-array-check"
		;;
	'-effect-gen')
		OPT_PASSES="-effect-gen"
		;;
	'-test-pass')
		OPT_PASSES="-test-pass"
		;;
	'-very-busy-analysis')
		OPT_PASSES="-very-busy-analysis"
		;;
	'-modify-check')
		OPT_PASSES="-modify-check"
		;;	
	'-available-analysis')
		OPT_PASSES="-available-analysis"
		;;
	'-remove-global')
		OPT_PASSES="-remove-global"
		;;
	*)
		echo "invalid argument,"
		echo "Usage: runPass	[ -array-check | -effect-gen | -very-busy-analysis | -modify-check | -available-analysis | -remove-global  ]"
		exit 1
		;;
esac

#remove suffix .c
TEST_NAME=${2%.c}

#compiles the test file, and the check library
clang -emit-llvm -S -o $TEST_NAME.ll $TEST_NAME.c
clang++ -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -emit-llvm -S -o LibArrayCheck.ll LibArrayCheck.cpp

#run opt, with specified passes
./$OPT -load $MODULE_LIB $OPT_PASSES -debug-pass=Structure -S -o $TEST_NAME.mod.ll < $TEST_NAME.ll > /dev/null


#the test code and library code are linked into executable
clang -lstdc++ -o $TEST_NAME.out $TEST_NAME.mod.ll LibArrayCheck.ll

#clean up al intermediate files, unless specified
#if [[ $1 != "-i" ]] 
#then
#	rm -rf *.bc *.ll *.o
#fi
	
