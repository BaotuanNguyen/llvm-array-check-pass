#!/bin/bash

opt -load ../../Release+Asserts/lib/llvm-array-check-pass.so -hello < test.bc > /dev/null
