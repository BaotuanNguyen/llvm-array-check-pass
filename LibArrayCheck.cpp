#include <iostream>


extern "C" void check(char* str)
{
	std::cout << "check " << str << "\n";
}

