#!/bin/bash

TEST_FILE=$2
OPT_PASSES=""
LLVM_LIBRARY=../../Release+Asserts/
MODULE_LIB=`ls ${LLVM_LIBRARY}lib/llvm-array-check-pass*`
OPT="${LLVM_LIBRARY}bin/opt"
MOREOPTION=""
LLFILE=""

#other optimizations can added by opt as argument
#all optimization can be obtained by -help

#DIFFERENT WAYS TO RUN OPT UNDER
case "$1" in
	'-insert-check')	
		OPT_PASSES="-insert-check"
		;;
	'-loop-hoist')
		OPT_PASSES="-loop-hoist"
		;;
	'-very-busy-analysis')
		OPT_PASSES="-insert-check -very-busy-analysis"
		;;
	'-modify-check')
		OPT_PASSES="-insert-check -modify-check"
		;;	
	'-available-analysis')
		OPT_PASSES="-insert-check -modify-check -available-analysis"
		;;
	'-remove-check')
		OPT_PASSES="-insert-check -modify-check -remove-check" 
		;;
	'-remove-check-hoist')
		OPT_PASSES="-insert-check -loop-hoist -modify-check -remove-check" 
		;;

	'-remove-check-more')
		OPT_PASSES="-insert-check -modify-check -remove-check" MOREOPTION="-D__MORE__"
		;;
	*)
		echo "invalid argument,"
		echo "Usage: runPass	[ -effect-gen | -insert-check | -loop-hoist | -very-busy-analysis | -modify-check
	   	| -available-analysis | -remove-check ] <filename>"
		exit 1
		;;
esac

#remove suffix .c
SUFFIX=${2##*.}
SUFFIX=.$SUFFIX

TEST_NAME=${2%$SUFFIX}

touch VeryBusyAnalysisPass.cpp
touch AvailableAnalysisPass.cpp
make CPPFLAGS=$MOREOPTION

#compiles the test file, and the check library

clang -emit-llvm -S -o $TEST_NAME.ll $TEST_NAME$SUFFIX

#clang++ -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -D__MORE__=$MORE -emit-llvm -S -o LibArrayCheck.ll LibArrayCheck.cpp
clang++ -emit-llvm -S -o LibArrayCheck.ll LibArrayCheck.cpp

./$OPT -load $MODULE_LIB $OPT_PASSES -debug-pass=Structure -S -disable-verify -o $TEST_NAME.mod.ll < $TEST_NAME.ll > /dev/null

#the test code and library code are linked into executable
#clang -lstdc++ -o $TEST_NAME.out $TEST_NAME.mod.ll LibArrayCheck.ll

#clean up al intermediate files, unless specified
#if [[ $1 != "-i" ]] 
#then
#	rm -rf *.bc *.ll *.o
#fi
