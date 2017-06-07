#include "WinsocModule.h"
#include "RakNetModule.h"
#include <process.h>

//Raknet
#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "GetTime.h"
#include "RakNetStatistics.h"

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
#define BIG_PACKET_SIZE 83296256



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
	int timetotal = 0;
	int high = -1;
	int lowest = 999999999;
	int iterations = 1;
	std::ofstream file;

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
						std::string filename = "";
						for (int i = 1; i < argc; i++)
						{
							filename.append(argv[i]);
							filename.append(" ");
						}
						std::ostringstream os;
						os << PING_ITERATIONS;
						file.open("../Logs/" + filename + " " + os.str() + ".tsv");
						file << filename << "\n";
						file << "Nummber of Iterations: " << PING_ITERATIONS << "\n";
						//Take avg delay of the connection
						for (int i = 0; i < 5; i++)
						{
							switch (i)
							{
							case 0:
								file << "4 bytes:\n";
								avgDelayNS = wsModule.Calculate_AVG_Delay(4);
								break;
							case 1:
								file << "512 bytes:\n";
								avgDelayNS = wsModule.Calculate_AVG_Delay(512);
								break;
							case 2:
								file << "1024 bytes:\n";
								avgDelayNS = wsModule.Calculate_AVG_Delay(1024);
								break;
							case 3:
								file << "1500 bytes:\n";
								avgDelayNS = wsModule.Calculate_AVG_Delay(1500);
								break;
							case 4:
								file << "2048 bytes:\n";
								avgDelayNS = wsModule.Calculate_AVG_Delay(2048);
								break;
							}

							printf("Average Delay: %d ns, ", avgDelayNS);
							printf("Highest Delay: %d ns, ", wsModule.GetHighest());
							printf("Lowest Delay: %d ns \n", wsModule.GetLowest());

							file << "AvrageDelay (ns)	HighestDelay (ns)	LowestDelay (ns)\n";
							file << avgDelayNS << "	" << wsModule.GetHighest() << "	" << wsModule.GetLowest() << "\n";
							file << "\n";
						}
						file.close();
					}
					else //Time data
					{

						for (int i = 0; i < iterations; i++)
						{
							//Send data
							timeMS = wsModule.TCP_Send_Data();

							timetotal += timeMS;
							if (timeMS > high)
							{
								high = timeMS;
							}

							if (timeMS < lowest)
							{
								lowest = timeMS;
							}
						}
						printf("Average time: %d, Highest Time: %d, Lowest Time: %d\n", timetotal / iterations, high, lowest);		
						
						std::string filename = "";
						for (int i = 1; i < argc; i++)
						{
							filename.append(argv[i]);
							filename.append(" ");
						}

						file.open("../Logs/" + filename + ".tsv");
						file << filename << "\n";
						file << "Packet Size: " << TCP_PACKET_SIZE << " KB\n";
						file << "AverageTime (ms)	HighestTime (ms)	LowestTime (ms)\n";
						file << timetotal / iterations << "	" << high << "	" << lowest << "\n";
						file.close();
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
			else //UDP
			{
				if (isSender)	//Is set to be the sender
				{
					if (ping == true)	//Calculate delay
					{
						std::string filename = "";
						for (int i = 1; i < argc; i++)
						{
							filename.append(argv[i]);
							filename.append(" ");
						}
						std::ostringstream os;
						os << PING_ITERATIONS;
						file.open("../Logs/" + filename + " " + os.str() + ".tsv");
						file << filename << "\n";
						file << "Nummber of Iterations: " << PING_ITERATIONS << "\n";
						//Take avg delay of the connection
						for (int i = 0; i < 5; i++)
						{
							switch(i)
							{
								case 0:
									file << "4 bytes:\n";
									avgDelayNS = wsModule.Calculate_AVG_Delay(ip, 4);
									break;
								case 1:
									file << "512 bytes:\n";
									avgDelayNS = wsModule.Calculate_AVG_Delay(ip, 512);
									break;
								case 2:
									file << "1024 bytes:\n";
									avgDelayNS = wsModule.Calculate_AVG_Delay(ip, 1024);
									break;
								case 3:
									file << "1500 bytes:\n";
									avgDelayNS = wsModule.Calculate_AVG_Delay(ip, 1500);
									break;
								case 4:
									file << "2048 bytes:\n";
									avgDelayNS = wsModule.Calculate_AVG_Delay(ip, 2048);
									break;
							}
								
							printf("Average Delay: %d ns, ", avgDelayNS);
							printf("Highest Delay: %d ns, ", wsModule.GetHighest());
							printf("Lowest Delay: %d ns ", wsModule.GetLowest());
							printf("Loss: %f \n", (double)wsModule.GetLost()/PING_ITERATIONS);
							std::ostringstream os;
							os << (double)wsModule.GetLost() / PING_ITERATIONS;
							file << "AvrageDelay (ns)	HighestDelay (ns)	LowestDelay (ns)	Loss(%): \n";
							file << avgDelayNS << "	" << wsModule.GetHighest() << "	" << wsModule.GetLowest() <<"	" << os.str() << "\n";
							file << "\n";
						}
						file.close();
					}
					else //Time data
					{
						int high = -1;
						int lowest = 999999999;
						wsModule.Clear_PacketLoss_Vector();
						for (int i = 0; i < iterations; i++)
						{
							//Send data
							timeMS = wsModule.UDP_Send_Data(ip, i);

							timetotal += timeMS;
							if (timeMS > high)
							{
								high = timeMS;
							}

							if (timeMS < lowest)
							{
								lowest = timeMS;
							}
							
						}
						printf("Average time: %d, Highest Time: %d, Lowest Time: %d\n", timetotal / iterations, high, lowest);

						wsModule.Calcualet_Loss();
						printf("Average Loss: %f, ", wsModule.GetAverageLoss());
						printf("Highest Loss: %f, ", wsModule.GetHighestLoss());
						printf("Lowest Loss: %f \n", wsModule.GetLowestLoss());

						std::string filename = "";
						for (int i = 1; i < argc; i++)
						{
							filename.append(argv[i]);
							filename.append(" ");
						}

						file.open("../Logs/" + filename + ".tsv");
						file << filename << "\n";
						file << "Packet Size: " << UDP_PACKET_SIZE << " KB\n";
						file << "AverageTime (ms)	HighestTime (ms)	LowestTime (ms)\n";
						file << timetotal / iterations << "	" << high << "	" << lowest << "\n";
						file << "\n";
						file << "AverageLoss	HighestLoss	LowestLoss\n";
						std::ostringstream os;
						os << (double)wsModule.GetAverageLoss() << "	" << (double)wsModule.GetHighestLoss() << "	" << (double)wsModule.GetLowestLoss() << "\n";
						file << os.str();

						file.close();
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

#pragma region
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


				if (ping)
				{
					std::string filename = "";
					for (int i = 1; i < argc; i++)
					{
						filename.append(argv[i]);
						filename.append(" ");
					}
					std::ostringstream os;
					os << PING_ITERATIONS;
					file.open("../Logs/" + filename + " " + os.str() + ".tsv");
					file << filename << "\n";
					file << "Nummber of Iterations: " << PING_ITERATIONS << "\n";
					//Take avg delay of the connection
					for (int i = 0; i < 5; i++)
					{
						switch (i)
						{
						case 0:
							file << "4 bytes:\n";
							avgDelayNS = rnModule.Calculate_AVG_Delay(4);
							break;
						case 1:
							file << "512 bytes:\n";
							avgDelayNS = rnModule.Calculate_AVG_Delay(512);
							break;
						case 2:
							file << "1024 bytes:\n";
							avgDelayNS = rnModule.Calculate_AVG_Delay(1024);
							break;
						case 3:
							file << "1500 bytes:\n";
							avgDelayNS = rnModule.Calculate_AVG_Delay(1500);
							break;
						case 4:
							file << "2048 bytes:\n";
							avgDelayNS = rnModule.Calculate_AVG_Delay(2048);
							break;
						}

						printf("Average Delay: %d ns, ", avgDelayNS);
						printf("Highest Delay: %d ns, ", rnModule.GetHighest());
						printf("Lowest Delay: %d ns \n", rnModule.GetLowest());

						file << "AvrageDelay (ns)	HighestDelay (ns)	LowestDelay (ns)\n";
						file << avgDelayNS << "	" << rnModule.GetHighest() << "	" << rnModule.GetLowest() << "\n";
						file << "\n";
					}
					file.close();
				}
				else
				{
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
					printf("Total Time: %d\n", timeMS);
				}

			}
			else
			{
				if (ping)
				{
					while (true)
						rnModule.Update();
				}
				else
				{
					while (true)
						rnModule.WaitForData();
				}
				
			}
			
			rnModule.Shutdown();
		}

#pragma endregion OldRakNet

	}
	else
	{
		printf("Failed to initialize the program because of bad parameters");
	}
	
	printf("Ending program \n");
	
	return 0;
}