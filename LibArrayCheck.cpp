#include <iostream>

extern "C" void check(char* str, int type, int index, int limit)
{
	//std::cout << "check " << str << ", " << type << ", " << index << ", " << limit << "\n";
	switch(type)
	{
		//die
		case 0:
			std::cout << "die()\n";
			std::terminate();
			break;
		//index >= 0
		case 1:
			std::cout << "checking " << str << " : " << index << " >= 0\n";
			if(index < 0)
			{
				std::cout << str << ": " << index << " < 0\n";
				std::terminate();
			}
			break;
		case 2:
		//index < limit
			std::cout << "checking " << str << " : " << index << " < " << limit << "\n";
			if(index >= limit)
			{
				std::cout << str << ": " << index << " >= limit\n";
				std::terminate();
			}
			break;
	}
}

