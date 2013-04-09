#include <stdlib.h>
#include <iostream>
#include <stdint.h>

extern "C" void checkLessThan(int64_t left, int64_t right)
{
	//std::cout << "checking: " << left << " < " << right << "\n";
	
	if (left >= right)
	{
		std::cout << "DETECT: " << left << " >= " << right << "\n";
		std::cout << "Run-time Analysis detected an out-of-bound access! Terminating...\n";
//		exit(1);
	}
}

