
#include <iostream>
#include "WinsocModule.h"
#include "RakNetModule.h"
#include "PCapModule.h"

int main(int argc, char **argv)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	
	Protocol p = Protocol::TCP_WITH_NODELAY;
	std::string filterString = "";
	std::string filename = "packetData.pcap";

	bool connect = false;
	WinsocModule wsModule;
	RakNetModule rnModule;



	
	
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

	printf("Ending program \n");

	return 0;
}