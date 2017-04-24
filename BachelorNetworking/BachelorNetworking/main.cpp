//<> 

#include <iostream>
#include "WinsocModule.h"

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	WinsocModule wsModule;
	
	wsModule.Initialize();
	
	wsModule.Connect("192.168.99.5");
	
	wsModule.Shutdown();

	

	return 0;
}