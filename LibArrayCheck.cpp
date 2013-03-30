#include <iostream>


extern "C" void check(char* str, int lowerBound, int index, int upperBound)
{
	std::cout << "check " << str << ", " << lowerBound << ", " << index << ", " << upperBound << "\n";
}

