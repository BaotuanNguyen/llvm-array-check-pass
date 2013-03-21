#/bin/bash -x -v
#
#linking 
#
#	test.c (C code) to
#	LibArrayCheck.cpp (C++ code)
#	test (application code)
#
#clang -emit-llvm -c -o  test.bc test.c
clang++ -emit-llvm -c -o  LibArrayCheck.bc LibArrayCheck.cpp
clang -lstdc++ -o "test" test1.bc LibArrayCheck.bc
rm -rf *.bc *.ll *.o
