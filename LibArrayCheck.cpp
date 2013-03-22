#include <iostream>

extern "C" void scopeStart()
{
	std::cout << "started scope";
}
extern "C" void arrayAccess(int variableId)
{
	std::cout << "Hello World\n";
}
extern "C" void alloca(char* str)
{
	std::cout << str << "\n";
}
extern "C" void scopeEnd()
{
	std::cout << "ended scope";
}

