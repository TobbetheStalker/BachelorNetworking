#include <iostream>
#include "WinsocModule.h"
#include "RakNetModule.h"
#include <process.h>

#pragma warning(disable:4789)
#pragma comment(lib, "pdh.lib")

CONST LPCSTR COUNTER_PATH = "\\Network Interface(Realtek PCI GBE Family Controller)\\Bytes Sent/sec";
//"\\Processor(0)\\% Processor Time";
CONST ULONG SAMPLE_INTERVAL_MS = 1000;
Protocol p = Protocol::NONE;
std::string filename = "log";
char* ip = "";
bool isSender = false;
bool ping = false;



bool SetParam(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++)
	{
		
		std::string arg = (std::string)argv[i];
		
		if (arg == "-s")	//Sender
		{
			// It is the sender who will record the time it takes
			isSender = true;
		}
		else if (arg == "-r")	//Reciver
		{
			isSender = false;
		}
		else if (arg == "-t")	//TCP
		{
			if (p == Protocol::NONE)
			{
				p = Protocol::TCP;
			}
			else
			{
				printf("Protocol has already been set");
				return 0;
			}
		}
		else if (arg == "-tn")	//TCP no delay
		{
			if (p == Protocol::NONE)
			{
				p = Protocol::TCP_WITH_NODELAY;
			}
			else
			{
				printf("Protocol has already been set");
				return 0;
			}
		}
		else if (arg == "-u")	//UDP
		{
			if (p == Protocol::NONE)
			{
				p = Protocol::UDP;
			}
			else
			{
				printf("Protocol has already been set");
				return 0;
			}
		}
		else if (arg == "-rn")	//RakNet
		{
			if (p == Protocol::NONE)
			{
				p = Protocol::RAKNET;
			}
			else
			{
				printf("Protocol has already been set");
				return 0;
			}
		}
		else if (arg == "-ip")	//IP
		{
			// We will assume people will provide an ip address and nothing else

			if (i + 1 < argc)	//Check so this is not the final parameter
			{
				ip = argv[i+1];	//Read the next parameter into ip
			}
			else
			{
				printf("-ip needs to be followed by an ip address");
				return 0;
			}
		}
		else if (arg == "-ln")	//log name
		{
			if (i + 1 < argc)	//Check so this is not the final parameter
			{
				filename = argv[i+1];	//Read the next parameter into ip
				filename.append(".txt");
			}
			else
			{
				printf("-ln needs to be followed by the filename");
				return 0;
			}
		}
		else if (arg == "-p")
		{
			ping = true;
		}
	}


	/*
	Final check to see if the crucial parameters
	has been set to valid values
	*/

	if (p == Protocol::NONE && ip == "")
	{
		return 0;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	bool run = true;
	bool conrq = false;
	bool calcDelay = false;
	int avgDelayNS = 0;
	int timeMS = 0;
	int totalTimeNS = 0;

	if (SetParam(argc, argv)) 
	{
		WinsocModule wsModule;
		RakNetModule rnModule;
	
		if (p != Protocol::RAKNET)	//Winsoc
		{
#pragma region
			wsModule.Initialize(p);

			if (p == Protocol::TCP || p == Protocol::TCP_WITH_NODELAY)
			{

				if (isSender)	//Is set to be the sender
				{
					
					wsModule.TCP_Connect(ip);
					
					//Wait for connection, not really needed since TCP blocks until connection
					while (wsModule.GetIsConnected() != true)
					{
						wsModule.Update();
					}
				

					if (ping == true)	//Calculate delay
					{
						//Take avg delay of the connection
						avgDelayNS = wsModule.Calculate_AVG_Delay();
						printf("Average Delay: %d ns", avgDelayNS);
					}
					else //Time data
					{
						//Start Timer
						wsModule.Clock_Start();

						//Send data
						wsModule.TCP_Send_Data();

						//Recive Last ack
						while (wsModule.GetTransferComplete() == false)
						{
							wsModule.TCP_Update();
						}

						//Stop timer
						timeMS = wsModule.Clock_Stop(true);
						printf("Average Delay: %d ms\n", timeMS);
					}
							
						
				}
				else //Is set to be reciver
				{
					while (wsModule.GetIsConnected() != true)
					{
						wsModule.Update();

					}

					if (ping == true)
					{
						while(true)
							wsModule.TCP_Update(); //Only need to update
					}
					else
					{
						while (true)
							wsModule.TCP_WaitForData(); //Only need to update
					}
					
				}

			}
			else 
			{
				if (isSender)	//Is set to be the sender
				{

					if (ping == true)	//Calculate delay
					{
						//Take avg delay of the connection
						avgDelayNS = wsModule.Calculate_AVG_Delay(ip);
						printf("Average Delay: %d ns", avgDelayNS);
					}
					else //Time data
					{
						//Start Timer
						wsModule.Clock_Start();

						//Send data
						wsModule.UDP_Send_Data(ip);

						//Recive Last ack
						while (wsModule.GetTransferComplete() == false)
						{
							wsModule.UDP_Update();
						}

						//Stop timer
						timeMS = wsModule.Clock_Stop(true);
						printf("Total time: %d ms\n", timeMS);
					}


				}
				else //Is set to be reciver
				{
					if (ping == true)
					{
						while (true)
							wsModule.UDP_Update(); //Only need to update
					}
					else
					{
						while (true)
							wsModule.UDP_WaitForData(); //Only need to update
					}

				}
			}

			wsModule.Shutdown();
#pragma endregion Winsoc
		
		}
		else //RakNet
		{
			rnModule.Initialize();

			if (isSender)
			{
				rnModule.Connect(ip);

				//Wait until handshake is completed
				while (rnModule.GetIsConnected() != true)
				{
					rnModule.Update();
				}

				//Take avg delay of the connection
				avgDelayNS = rnModule.Calculate_AVG_Delay();

				//Start Timer
				rnModule.Clock_Start();

				//Send data
				rnModule.SendData();

				//Recive Last ack
				while (rnModule.GetTransferComplete() == false)
				{
					rnModule.Update();
				}

				//Stop timer
				timeMS = rnModule.Clock_Stop(true);

				//Take time - avg delay
				totalTimeNS = timeMS - avgDelayNS;
				printf("Total Time: %d \n avgDelay: %d\n", totalTimeNS, avgDelayNS);

			}
			else
			{
				while (true)
					rnModule.Update();
			}
			
			rnModule.Shutdown();
		}

	}
	else
	{
		printf("Failed to initialize the program because of bad parameters");
	}
	
	printf("Ending program \n");
	
	return 0;
}