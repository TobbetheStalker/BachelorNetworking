#include <iostream>
#include <fstream>
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
	int iterations = 5;
	std::ofstream file;

	if (SetParam(argc, argv)) 
	{
		WinsocModule wsModule;
		//RakNetModule rnModule;
	
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
						//Take avg delay of the connection (one way)
						avgDelayNS = wsModule.Calculate_AVG_Delay();
						printf("Average Delay: %d ns, ", avgDelayNS);
						printf("Highest Delay: %d ns, ", wsModule.GetHighest());
						printf("Lowest Delay: %d ns\n", wsModule.GetLowest());
						
						std::string filename = "";
						for (int i = 1; i < argc; i++)
						{
							filename.append( argv[i]);
							filename.append(" ");
						}

						file.open("../Logs/" + filename + ".tsv");
						file << filename;
						file << "\n";
						file << "AverageDelay (ns)	HighestDelay (ns)	LowestDelay (ns)\n";
						file << avgDelayNS << "	" << wsModule.GetHighest() << "	" << wsModule.GetLowest() << "\n";
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
						file << filename;
						file << "\n";
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
						file.open("../Logs/" + filename + ".tsv");
						file << filename << "\n";
						file << "Nummber of Iterations: " << PING_ITERATIONS;
						file << "\n";
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
							printf("Lowest Delay: %d ns\n", wsModule.GetLowest());
								
							file << "AvrageDelay (ns)	HighestDelay (ns)	LowestDelay (ns)\n";
							file << avgDelayNS << "	" << wsModule.GetHighest() << "	" << wsModule.GetLowest() << "\n";
							file << "\n";
						}
						file.close();
					}
					else //Time data
					{
						wsModule.Clear_PacketLoss_Vector();
						for (int i = 0; i < iterations; i++)
						{
							//Send data
							timeMS = wsModule.UDP_Send_Data(ip);

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
						printf("Average time: %d, Highest Time: %d, Lowest Time: %d\n", timetotal/iterations, high, lowest);
	
						wsModule.Calcualet_Loss();
						printf("Average Loss: %f, ", wsModule.GetAverageLoss());
						printf("Highest Loss: %d, ", wsModule.GetHighestLoss());
						printf("Lowest Loss: %d \n", wsModule.GetLowestLoss());

						std::string filename = "";
						for (int i = 1; i < argc; i++)
						{
							filename.append(argv[i]);
							filename.append(" ");
						}

						file.open("../Logs/" + filename + ".tsv");
						file << filename << "\n";
						file << "AverageTime (ms)	HighestTime (ms)	LowestTime (ms)\n";
						file << timetotal / iterations << "	" << high << "	" << lowest << "\n";
						file << "\n";
						file << "AverageLoss	HighestLoss	LowestLoss\n";
						file << wsModule.GetAverageLoss() << "	" << wsModule.GetHighestLoss() << "	" << wsModule.GetLowestLoss() << "\n";

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


//		RakNet::RakPeerInterface *client, *server;
//		char *data = new char[BIG_PACKET_SIZE];;
//		int socketFamily = AF_INET;
//		RakNet::TimeMS start, stop;
//		RakNet::TimeMS nextStatTime = RakNet::GetTimeMS() + 1000;
//		RakNet::Packet *packet;
//		start = RakNet::GetTimeMS();
//#pragma region
//
//		if (isSender) //Sender
//		{
//			client = RakNet::RakPeerInterface::GetInstance();
//
//			client->SetTimeoutTime(5000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
//			RakNet::SocketDescriptor socketDescriptor(0, 0);
//			socketDescriptor.socketFamily = socketFamily;
//			RakNet::StartupResult sr;
//			sr = client->Startup(4, &socketDescriptor, 1);
//			if (sr != RakNet::RAKNET_STARTED)
//			{
//				printf("Client failed to start. Error=%i\n", sr);
//				return 1;
//			}
//			client->SetSplitMessageProgressInterval(10000); // Get ID_DOWNLOAD_PROGRESS notifications
//															//	client->SetPerConnectionOutgoingBandwidthLimit(28800);
//
//			printf("Started client on %s\n", client->GetMyBoundAddress().ToString(true));
//
//			client->Connect(data, 6881, 0, 0);
//
//		}
//		else //Reciver
//		{
//			server = RakNet::RakPeerInterface::GetInstance();
//
//			server->SetTimeoutTime(5000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);
//			RakNet::SocketDescriptor socketDescriptor(6881, 0);
//			socketDescriptor.socketFamily = socketFamily;
//			server->SetMaximumIncomingConnections(4);
//			RakNet::StartupResult sr;
//			sr = server->Startup(4, &socketDescriptor, 1);
//			if (sr != RakNet::RAKNET_STARTED)
//			{
//				printf("Server failed to start. Error=%i\n", sr);
//				return 1;
//			}
//			// server->SetPerConnectionOutgoingBandwidthLimit(40000);
//
//			printf("Started server on %s\n", server->GetMyBoundAddress().ToString(true));
//		}
//
//		printf("My IP addresses:\n");
//		RakNet::RakPeerInterface *rakPeer;
//		if (server)
//			rakPeer = server;
//		else
//			rakPeer = client;
//		unsigned int i;
//		for (i = 0; i < rakPeer->GetNumberOfAddresses(); i++)
//		{
//			printf("%i. %s\n", i + 1, rakPeer->GetLocalIP(i));
//		}
//#pragma endregion Init
//
//#pragma region
//
//		while (true)
//		{
//
//			if (isSender) //Client
//			{
//				packet = client->Receive();
//				while (packet)
//				{
//					if (packet->data[0] == ID_DOWNLOAD_PROGRESS)
//					{
//						RakNet::BitStream progressBS(packet->data, packet->length, false);
//						progressBS.IgnoreBits(8); // ID_DOWNLOAD_PROGRESS
//						unsigned int progress;
//						unsigned int total;
//						unsigned int partLength;
//
//						// Disable endian swapping on reading this, as it's generated locally in ReliabilityLayer.cpp
//						progressBS.ReadBits((unsigned char*)&progress, BYTES_TO_BITS(sizeof(progress)), true);
//						progressBS.ReadBits((unsigned char*)&total, BYTES_TO_BITS(sizeof(total)), true);
//						progressBS.ReadBits((unsigned char*)&partLength, BYTES_TO_BITS(sizeof(partLength)), true);
//
//						printf("Progress: msgID=%i Progress %i/%i Partsize=%i\n",
//							(unsigned char)packet->data[0],
//							progress,
//							total,
//							partLength);
//					}
//					else if (packet->data[0] == 255)
//					{
//						if (packet->length != BIG_PACKET_SIZE) //Size check
//						{
//							printf("Test failed. %i bytes (wrong number of bytes).\n", packet->length);
//							//quit = true;
//							break;
//						}
//						if (BIG_PACKET_SIZE <= 100000)	//Data check
//						{
//							for (int i = 0; i < BIG_PACKET_SIZE; i++)
//							{
//								if (packet->data[i] != 255 - (i & 255))
//								{
//									printf("Test failed. %i bytes (bad data).\n", packet->length);
//									//quit = true;
//									break;
//								}
//							}
//						}
//
//					}
//					else if (packet->data[0] == 254)
//					{
//						printf("Got high priority message.\n");
//					}
//					else if (packet->data[0] == ID_CONNECTION_LOST)
//						printf("ID_CONNECTION_LOST from %s\n", packet->systemAddress.ToString());
//					else if (packet->data[0] == ID_DISCONNECTION_NOTIFICATION)
//						printf("ID_DISCONNECTION_NOTIFICATION from %s\n", packet->systemAddress.ToString());
//					else if (packet->data[0] == ID_NEW_INCOMING_CONNECTION)
//						printf("ID_NEW_INCOMING_CONNECTION from %s\n", packet->systemAddress.ToString());
//					else if (packet->data[0] == ID_CONNECTION_REQUEST_ACCEPTED)
//					{
//						start = RakNet::GetTimeMS();
//						printf("ID_CONNECTION_REQUEST_ACCEPTED from %s\n", packet->systemAddress.ToString());
//					}
//					else if (packet->data[0] == ID_CONNECTION_ATTEMPT_FAILED)
//						printf("ID_CONNECTION_ATTEMPT_FAILED from %s\n", packet->systemAddress.ToString());
//
//					client->DeallocatePacket(packet);
//					packet = client->Receive();
//				}
//			}
//			else //Server
//			{
//				for (packet = server->Receive(); packet; server->DeallocatePacket(packet), packet = server->Receive())
//				{
//					if (packet->data[0] == ID_NEW_INCOMING_CONNECTION || packet->data[0] == 253)
//					{
//						printf("Starting send\n");
//						start = RakNet::GetTimeMS();
//						if (BIG_PACKET_SIZE <= 100000)
//						{
//							for (int i = 0; i < BIG_PACKET_SIZE; i++)
//								data[i] = 255 - (i & 255);
//						}
//						else
//						{
//							data[0] = (unsigned char)255;
//						}
//
//						server->Send(data, BIG_PACKET_SIZE, LOW_PRIORITY, RELIABLE_ORDERED_WITH_ACK_RECEIPT, 0, packet->systemAddress, false);
//						// Keep the stat from updating until the messages move to the thread or it quits right away
//						nextStatTime = RakNet::GetTimeMS() + 1000;
//					}
//					if (packet->data[0] == ID_CONNECTION_LOST)
//						printf("ID_CONNECTION_LOST from %s\n", packet->systemAddress.ToString());
//					else if (packet->data[0] == ID_DISCONNECTION_NOTIFICATION)
//						printf("ID_DISCONNECTION_NOTIFICATION from %s\n", packet->systemAddress.ToString());
//					else if (packet->data[0] == ID_NEW_INCOMING_CONNECTION)
//						printf("ID_NEW_INCOMING_CONNECTION from %s\n", packet->systemAddress.ToString());
//					else if (packet->data[0] == ID_CONNECTION_REQUEST_ACCEPTED)
//						printf("ID_CONNECTION_REQUEST_ACCEPTED from %s\n", packet->systemAddress.ToString());
//				}
//			}
//
//		}
//
//#pragma region
//		if (RakNet::GetTimeMS() > nextStatTime)
//		{
//			nextStatTime = RakNet::GetTimeMS() + 1000;
//			RakNet::RakNetStatistics rssSender;
//			RakNet::RakNetStatistics rssReceiver;
//			if (server)
//			{
//				unsigned int i;
//				unsigned short numSystems;
//				server->GetConnectionList(0, &numSystems);
//				if (numSystems>0)
//				{
//					for (i = 0; i < numSystems; i++)
//					{
//						server->GetStatistics(server->GetSystemAddressFromIndex(i), &rssSender);
//						StatisticsToString(&rssSender, data, 2);
//						printf("==== System %i ====\n", i + 1);
//						printf("%s\n\n", data);
//					}
//				}
//			}
//			if (client && server == 0 && client->GetGUIDFromIndex(0) != RakNet::UNASSIGNED_RAKNET_GUID)
//			{
//				client->GetStatistics(client->GetSystemAddressFromIndex(0), &rssReceiver);
//				StatisticsToString(&rssReceiver, data, 2);
//				printf("%s\n\n", data);
//			}
//		}
//#pragma endregion UpdateStatistics
//
//#pragma endregion Update
//
//		stop = RakNet::GetTimeMS();
//		double seconds = (double)(stop - start) / 1000.0;
//
//		if (server)
//		{
//			RakNet::RakNetStatistics *rssSender2 = server->GetStatistics(server->GetSystemAddressFromIndex(0));
//			StatisticsToString(rssSender2, data, 1);
//			printf("%s", data);
//		}
//
//		printf("%i bytes per second (%.2f seconds). Press enter to quit\n", (int)((double)(BIG_PACKET_SIZE) / seconds), seconds);
//		//Gets(data, BIG_PACKET_SIZE);
//
//		delete[]data;
//		RakNet::RakPeerInterface::DestroyInstance(client);
//		RakNet::RakPeerInterface::DestroyInstance(server);


#pragma endregion RakNet


#pragma region
		//else //RakNet
		//{
		//	rnModule.Initialize();

		//	if (isSender)
		//	{
		//		rnModule.Connect(ip);

		//		//Wait until handshake is completed
		//		while (rnModule.GetIsConnected() != true)
		//		{
		//			rnModule.Update();
		//		}


		//		if (ping)
		//		{
		//			//Take avg delay of the connection
		//			avgDelayNS = rnModule.Calculate_AVG_Delay();
		//			printf("Average Delay: %d ns", avgDelayNS);
		//		}
		//		else
		//		{
		//			//Start Timer
		//			rnModule.Clock_Start();

		//			//Send data
		//			rnModule.SendData();

		//			//Recive Last ack
		//			while (rnModule.GetTransferComplete() == false)
		//			{
		//				rnModule.Update();
		//			}

		//			//Stop timer
		//			timeMS = rnModule.Clock_Stop(true);

		//			//Take time - avg delay
		//			printf("Total Time: %d\n", timeMS);
		//		}

		//	}
		//	else
		//	{
		//		if (ping)
		//		{
		//			while (true)
		//				rnModule.Update();
		//		}
		//		else
		//		{
		//			while (true)
		//				rnModule.WaitForData();
		//		}
		//		
		//	}
		//	
		//	rnModule.Shutdown();
		//}

#pragma endregion OldRakNet

	}
	else
	{
		printf("Failed to initialize the program because of bad parameters");
	}
	
	printf("Ending program \n");
	
	return 0;
}