LEVEL=../..

LIBRARYNAME=llvm-array-check-pass

LOADABLE_MODULE=1

#files to be compiled
SOURCES:= ArrayBoundsCheckPass.cpp RangeCheckExpression.cpp RangeCheckSet.cpp EffectGenPass.cpp GlobalOptimizationsOnArrayChecks.cpp

include $(LEVEL)/Makefile.common
