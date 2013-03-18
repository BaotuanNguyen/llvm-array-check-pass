#!/bin/bash -x


LLVM_LIBRARY=../../Release+Asserts/

#other optimizations can added by opt as argument
#all optimization can be obtained by -help

opt -load "$LLVM_LIBRARY"lib/llvm-array-check-pass.so -hello --debug-pass=Structure < test.bc > /dev/null
