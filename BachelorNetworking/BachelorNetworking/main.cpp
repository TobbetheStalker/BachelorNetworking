
#include <iostream>
#include "WinsocModule.h"
#include "RakNetModule.h"
#include "PCapModule.h"

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
	Protocol p = Protocol::TCP_WITH_NODELAY;
	bool connect = false;
	WinsocModule wsModule;
	RakNetModule rnModule;
	PCapModule pcModule;

	pcModule.Initialize();

	pcModule.SelectDevice(2);
	pcModule.StartCapture();

	pcModule.Shutdown();
	
	//wsModule.Initialize(p);
	//while(true)
	//{
	//	wsModule.Update();
	//	//wsModule.UDP_Send(TEST, "192.168.99.6");
	//	if (connect == false && p != Protocol::UDP)
	//	{
	//		wsModule.TCP_Connect("127.0.0.1");
	//		connect = true;
	//	}
	//}

	//wsModule.Shutdown();
	
	//rnModule.Initialize();
	//rnModule.Shutdown();


	printf("Ending program");

	return 0;
}