#include <stdlib.h>
#include <iostream>
#include <stdint.h>
#include "ArrayBoundsCheckPass.h"

extern "C" void check(char* str, int64_t type, int64_t index, int64_t limit)
{
	//std::cout << "check " << str << ", " << type << ", " << index << ", " << limit << "\n";
	switch(type)
	{
		// check that index >= 0
                // (if index is less than 0, then index is out of bounds)
		case LOWER:
			std::cout << "checking " << str << " : " << index << " >= 0\n";
			if(index < 0)
			{
				std::cout << str << ": " << index << " < 0\n";
				std::cout << "Run-time Analysis detected an out-of-bound access! Terminating...\n";
				exit(1);
			}
			break;
		// check that index < limit
                // (if index is equal to or greater than the limiti, then index is out of bounds)
		case UPPER:
			std::cout << "checking " << str << " : " << index << " < " << limit << "\n";
			if(index >= limit)
			{
				std::cout << str << ": " << index << " >= " << limit << "\n";
				std::cout << "Run-time Analysis detected an out-of-bound access! Terminating...\n";
				exit(1);
			}
			break;
	}
}
