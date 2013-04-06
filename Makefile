LEVEL=../..

LIBRARYNAME=llvm-array-check-pass

LOADABLE_MODULE=1

#files to be compiled

#SOURCES:= ArrayBoundsCheckPass.cpp RunTimeBoundsChecking.cpp LocalOptimizationsOnArrayChecks.cpp
SOURCES:= ArrayBoundsCheckPass.cpp EffectGenPass.cpp RangeCheckSet.cpp RangeCheckExpression.cpp VeryBusyAnalysisPass.cpp AvailableAnalysisPass.cpp ModifyCheckPass.cpp RemoveRedundantCheckPass.cpp LoopCheckPropagationPass.cpp

include $(LEVEL)/Makefile.common
