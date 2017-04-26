
#include <iostream>
#include "WinsocModule.h"

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
	Protocol p = Protocol::UDP;
	bool connect = false;
	WinsocModule wsModule;
	
	wsModule.Initialize(p);
	
	
	
	while(true)
	{
		wsModule.Update();
		//wsModule.UDP_Send(TEST, "192.168.99.6");
		if (connect == false && p != Protocol::UDP)
		{
			wsModule.TCP_Connect("192.168.99.6");
			connect = true;
		}
	}


	wsModule.Shutdown();
	
	printf("Ending program");

	return 0;
}