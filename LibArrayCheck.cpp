#include <stdlib.h>
#include <iostream>
#include <stdint.h>

extern "C" void checkGTZero(int64_t index)
{
	std::cout << "checking: " << index << " >= 0\n";
	
	if(index < 0)
	{
		/*// check that index >= 0
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
                // (if index is equal to or greater than the limit, then index is out of bounds)
		case UPPER:
			std::cout << "checking " << str << " : " << index << " < " << limit << "\n";
			if(index >= limit)
			{
				std::cout << str << ": " << index << " >= " << limit << "\n";
				std::cout << "Run-time Analysis detected an out-of-bound access! Terminating...\n";
				exit(1);
			}
			break;*/
		std::cout << "DETECT:" << index << " < 0\n";
		std::cout << "Run-time Analysis detected an out-of-bound access! Terminating...\n";
		exit(1);
>>>>>>> 04a9b6785b2d8e0479a5bc43139f5db279fa2e41
	}
}

extern "C" void checkLTLimit(int64_t index, int64_t limit)
{
	std::cout << "checking: " << index << " < " << limit << "\n";
	
	if(index >= limit)
	{
		std::cout << "DETECT: " << index << " >= " << limit << "\n";
		std::cout << "Run-time Analysis detected an out-of-bound access! Terminating...\n";
		exit(1);
	}
}

