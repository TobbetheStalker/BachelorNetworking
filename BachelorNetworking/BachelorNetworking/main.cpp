
#include <iostream>
#include "WinsocModule.h"
#include "RakNetModule.h"
#include "PCapModule.h"
#include <pdh.h>
#include <pdhmsg.h>
#include <process.h>

#pragma comment(lib, "pdh.lib")

CONST LPCSTR COUNTER_PATH = "\\Network Interface(Realtek PCI GBE Family Controller)\\Bytes Sent/sec";
//"\\Processor(0)\\% Processor Time";
CONST ULONG SAMPLE_INTERVAL_MS = 1000;
Protocol p = Protocol::NONE;
std::string filename = "log";
char* ip = "";
bool isSender = false;
bool DoLog = true;
HQUERY hQuery = NULL;
	HLOG hLog = NULL;
	PDH_STATUS pdhStatus;
	DWORD dwLogType = PDH_LOG_TYPE_CSV;
	HCOUNTER hCounter;


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

void LogStart()
{
	// Open a query object.
	pdhStatus = PdhOpenQuery(NULL, 0, &hQuery);

	if (pdhStatus != ERROR_SUCCESS)
	{
		wprintf(L"PdhOpenQuery failed with 0x%x\n", pdhStatus);
		// Close the log file.
		if (hLog)
			PdhCloseLog(hLog, 0);

		// Close the query object.
		if (hQuery)
			PdhCloseQuery(hQuery);
	}

	// Add one counter that will provide the data.
	pdhStatus = PdhAddCounter(hQuery,
		COUNTER_PATH,
		0,
		&hCounter);

	if (pdhStatus != ERROR_SUCCESS)
	{
		wprintf(L"PdhAddCounter failed with 0x%x\n", pdhStatus);
		// Close the log file.
		if (hLog)
			PdhCloseLog(hLog, 0);

		// Close the query object.
		if (hQuery)
			PdhCloseQuery(hQuery);
	}

	// Open the log file for write access.

	pdhStatus = PdhOpenLog(filename.c_str(),
		PDH_LOG_WRITE_ACCESS | PDH_LOG_CREATE_ALWAYS,
		&dwLogType,
		hQuery,
		0,
		NULL,
		&hLog);

	if (pdhStatus != ERROR_SUCCESS)
	{
		wprintf(L"PdhOpenLog failed with 0x%x\n", pdhStatus);
		// Close the log file.
		if (hLog)
			PdhCloseLog(hLog, 0);

		// Close the query object.
		if (hQuery)
			PdhCloseQuery(hQuery);
	}
}

void Log( void*)
{
	int count = 0;
	// Write 10 records to the log file.
	while (DoLog && count || count == 0)
	{
		wprintf(L"Writing record \n");
		pdhStatus = PdhUpdateLog(hLog, NULL);
		if (ERROR_SUCCESS != pdhStatus)
		{
			wprintf(L"PdhUpdateLog failed with 0x%x\n", pdhStatus);
			goto cleanup;
		}
		count++;
		// Wait one second between samples for a counter update.
		Sleep(SAMPLE_INTERVAL_MS);
	}

cleanup:

	// Close the log file.
	if (hLog)
		PdhCloseLog(hLog, 0);

	// Close the query object.
	if (hQuery)
		PdhCloseQuery(hQuery);

	return;
}

int main(int argc, char *argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	bool run = true;
	bool conrq = false;
	bool calcDelay = false;
	int avgDelayNS = 0;
	int timeNS = 0;
	int totalTimeNS = 0;
	HANDLE threadH;
	DWORD threadID;

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

					wsModule.TCP_Update();
					
					if (wsModule.GetIsConnected())
					{
						//Take avg delay of the connection
						avgDelayNS = wsModule.Calculate_AVG_Delay();
							
						//Start log
						LogStart();
						threadH = (HANDLE)_beginthread(&Log, 0, (void*)0);

						//Start Timer
						wsModule.Clock_Start();

						//Send data
						wsModule.TCP_Send(TEST);

						//Recive Last ack
						while (wsModule.GetTransferComplete() == false)
						{
							wsModule.TCP_Update();
						}

						//Stop timer
						timeNS = wsModule.Clock_Stop();
						
						//Stop Log
						DoLog = false;
						WaitForSingleObject(threadH, INFINITE);
						
						//Take time - avg delay
						totalTimeNS = timeNS - avgDelayNS;
						printf("Total Time: %d \n avgDelay: %d\n", totalTimeNS, avgDelayNS);
						
					}


				}
				else //Is set to be reciver
				{
					wsModule.TCP_Update(); //Only need to update
				}

			}
			else 
			{
				if (isSender)	//Is set to be the sender
				{

					//Take avg delay of the connection
					avgDelayNS = wsModule.Calculate_AVG_Delay(ip);

					//Start log
					threadH = (HANDLE)_beginthread(&Log, 0, (void*)0);

					//Start Timer
					wsModule.Clock_Start();

					//Send data
					wsModule.UDP_Send(TEST, ip);

					//Recive Last ack
					while (wsModule.GetTransferComplete() == false)
					{
						wsModule.UDP_Update();
					}

					//Stop timer
					timeNS = wsModule.Clock_Stop();

					//Stop Log
					DoLog = false;
					WaitForSingleObject(threadH, INFINITE);

					//Take time - avg delay
					totalTimeNS = timeNS - avgDelayNS;
					printf("Total Time: %d \n avgDelay: %d\n", totalTimeNS, avgDelayNS);

					}
					else //Is set to be reciver
					{
						wsModule.UDP_Update(); //Only need to update
					}
			}

			wsModule.Shutdown();
#pragma endregion Winsoc
		
		}
		else //RakNet
		{
			rnModule.Initialize();

			rnModule.Connect(ip);

			//Take avg delay of the connection
			avgDelayNS = rnModule.Calculate_AVG_Delay();

			//Start log
			threadH = (HANDLE)_beginthread(&Log, 0, (void*)0);

			//Start Timer
			rnModule.Clock_Start();

			//Send data
			rnModule.Send(DefaultMessageIDTypes::R_TEST, TEST, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);

			//Recive Last ack
			while (rnModule.GetTransferComplete() == false)
			{
				rnModule.Update();
			}

			//Stop timer
			timeNS = rnModule.Clock_Stop();

			//Stop Log
			DoLog = false;
			WaitForSingleObject(threadH, INFINITE);

			//Take time - avg delay
			totalTimeNS = timeNS - avgDelayNS;
			printf("Total Time: %d \n avgDelay: %d\n", totalTimeNS, avgDelayNS);

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