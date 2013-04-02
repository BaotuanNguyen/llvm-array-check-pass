LEVEL=../..

LIBRARYNAME=llvm-array-check-pass

LOADABLE_MODULE=1

#files to be compiled
#SOURCES:= ArrayBoundsCheckPass.cpp RunTimeBoundsChecking.cpp LocalOptimizationsOnArrayChecks.cpp
SOURCES:= ArrayBoundsCheckPass.cpp RunTimeBoundsChecking.cpp LocalOptimizationsOnArrayChecks.cpp GlobalOptimizationsOnArrayChecks.cpp

include $(LEVEL)/Makefile.common
