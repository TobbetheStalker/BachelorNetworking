//<> 

#include <iostream>
#include "WinsocModule.h"

int main()
{
	
	WinsocModule wsModule;

	wsModule.Initialize();

	wsModule.Connect("192.168.99.5");

	return 0;
}