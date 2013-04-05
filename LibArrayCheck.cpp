#include <stdlib.h>
#include <iostream>
#include <stdint.h>

extern "C" void checkGTZero(int64_t index)
{
//	std::cout << "checking: " << index << " >= 0\n";
	
	if(index < 0)
	{
//		std::cout << "DETECT:" << index << " < 0\n";
//		std::cout << "Run-time Analysis detected an out-of-bound access! Terminating...\n";
		exit(1);
	}
}

extern "C" void checkLTLimit(int64_t index, int64_t limit)
{
//	std::cout << "checking: " << index << " < " << limit << "\n";
	
	if(index >= limit)
	{
//		std::cout << "DETECT: " << index << " >= " << limit << "\n";
//		std::cout << "Run-time Analysis detected an out-of-bound access! Terminating...\n";
		exit(1);
	}
}

