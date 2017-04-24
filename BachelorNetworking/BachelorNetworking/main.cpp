//<> 

#include <iostream>
#include "WinsocModule.h"

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
	Protocol p = Protocol::TCP_WITH_NODELAY;
	bool connect = false;
	WinsocModule wsModule;
	
	wsModule.Initialize(p);
	
	
	
	while(true)
	{
		wsModule.Update();

		if (connect == false)
		{
			wsModule.Connect("192.168.99.6");
			connect = true;
		}
	}


	wsModule.Shutdown();
	
	printf("Ending program");

	return 0;
}