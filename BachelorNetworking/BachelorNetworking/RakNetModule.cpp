#include "RakNetModule.h"

void RakNetModule::Clock_Start()
{
	// Set current time
	this->m_start_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();

	// Set to not send more pings since it will disrupt the timers
	this->m_ping_in_progress = true;
}

int RakNetModule::Clock_Stop()
{
	int result = 0;

	//Get the end time
	auto end_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();

	//Calculate the delta time togheter with end and start time to nano-seconds
	result = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - this->m_start_time).count();

	//Push back the result
	this->m_ping_times.push_back(result);

	//Set piong in progress to false
	this->m_ping_in_progress = false;

	return result;
}

bool RakNetModule::GetTransferComplete()
{
	return this->transferComplete;
}

bool RakNetModule::GetIsConnected()
{
	return this->isConnected;
}

float RakNetModule::GetAvrgRTT()
{
	float result = 0;
	int count = 0;
	std::vector<int>::iterator itr;

	for (itr = this->m_ping_times.begin(); itr != this->m_ping_times.end();)
	{
		result += *itr._Ptr;
		count++;
		itr++;
	}

	result = result / count;

	return result;
}

RakNetModule::RakNetModule()
{
	this->peer = nullptr;
	this->transferComplete = false;
	this->isConnected = false;
	this->dataCounter = 0;
}

RakNetModule::~RakNetModule()
{
}

bool RakNetModule::Initialize()
{
	this->peer = RakNet::RakPeerInterface::GetInstance();
	this->listner = RakNet::SocketDescriptor(6881, 0);
	
	this->peer->Startup(3, &this->listner, 1);
	this->peer->SetMaximumIncomingConnections(3);
	
	return 1;
}

void RakNetModule::Shutdown()
{
	this->peer->Shutdown(0);	//Time until shutdown to inform connected clients
	delete this->peer;
	this->peer = nullptr;
}

void RakNetModule::Update()
{
	RakNet::Packet* RaKpacket;
	RakNetPacket p;
	RakNetDataPacket* dp;

	for (RaKpacket = peer->Receive(); RaKpacket; peer->DeallocatePacket(RaKpacket), RaKpacket = peer->Receive())
	{

		//p.deserialize((char*)RaKpacket->data);

		switch (RaKpacket->data[0])
		{

		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			printf("Another client has disconnected.\n");
			break;
		case ID_REMOTE_CONNECTION_LOST:
			printf("Another client has lost the connection.\n");
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			printf("Another client has connected.\n");
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			printf("Our connection request has been accepted.\n");
			this->isConnected = true;
			break;
		case ID_NEW_INCOMING_CONNECTION:
			//this->Send(DefaultMessageIDTypes::R_START_PING, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
			printf("A connection is incoming.\n");
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			printf("The server is full.\n");
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			printf("A client has disconnected.\n");
			break;
		case ID_CONNECTION_LOST:
			printf("A client lost the connection.\n");
			break;

		case R_START_PING:
			printf("Recived R_CLOCK_SYNC Packet\n");
			this->Send(DefaultMessageIDTypes::R_CLOCK_SYNC, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
			break;

		case R_CLOCK_SYNC:
			printf("Recived R_CLOCK_SYNC Packet\n");
			this->Send(DefaultMessageIDTypes::R_CLOCK_SYNC_RESPONSE, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
			break;

		case R_CLOCK_SYNC_RESPONSE:
			printf("Recived R_CLOCK_SYNC_RESPONSE Packet\n");
			this->Clock_Stop();
			break;

		case R_CONNECTION_REQUEST:
			printf("Recived R_CONNECTION_REQUEST Packet\n");
			break;

		case R_DATA:
			printf("Recived R_TEST Packet\n");
			dp = (RakNetDataPacket*)RaKpacket->data;
			this->dataCounter++;
			if (this->dataCounter == dp->nrOfPackets)
			{
				this->Send(DefaultMessageIDTypes::R_TRANSFER_COMPLETE, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
			}
			break;

		case R_TRANSFER_COMPLETE :
			printf("Recived R_TRANSFER_COMPLETE Packet\n");
			this->transferComplete = true;

			break;

		default:
			printf("Unkown packet type %d\n", RaKpacket->data[0]);
			break;
		}
	}

}

bool RakNetModule::Connect(char * ip)
{
	int r = this->peer->Connect(ip, 6881, 0, 0);

	return 1;
}

void RakNetModule::Send(DefaultMessageIDTypes id, PacketPriority priority, PacketReliability reliability)
{

	RakNetPacket packet;
	packet.typeId = id;

	peer->Send(reinterpret_cast<char*>(&packet), sizeof(packet) , priority, reliability, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
}

void RakNetModule::SendData()
{
	RakNetDataPacket packet;
	packet.typeId = R_DATA;

	LARGE_INTEGER frequency, currTime, prevTime, elapsedTime;

	QueryPerformanceFrequency(&frequency);
	//QueryPerformanceCounter(&prevTime);
	QueryPerformanceCounter(&currTime);

	//1GB = 1073741824 bytes;
	int nrOfPackets = ceil(1073741824 / (sizeof(DataPacket)));
	const unsigned int packet_size = sizeof(DataPacket);
	packet.nrOfPackets = nrOfPackets;
	int counter = 0;
	elapsedTime.QuadPart = 0;

	while (counter <= nrOfPackets)
	{
		prevTime = currTime;
		QueryPerformanceCounter(&currTime);
		elapsedTime.QuadPart += currTime.QuadPart - prevTime.QuadPart;
		//elapsedTime.QuadPart /= 1000000;
		//elapsedTime.QuadPart /= frequency.QuadPart;

		//IF more than a secound has past
		if ((float)elapsedTime.QuadPart > 1000.f)
		{
			packet.ID = counter;
			peer->Send(reinterpret_cast<char*>(&packet), sizeof(packet), IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
			counter++;
			elapsedTime.QuadPart = 0;
			printf("Sent DataPacket %d\n", counter);
		}
	}

}

int RakNetModule::Calculate_AVG_Delay()
{
	/*
	1. Start a timer to measure teh RTT
	2. Send a CLOCK_SYNC packet which will trigger the reciver to send back a CLOCK_SYNC_RESPONSE packet
	with their original time
	3. Wait for the packet to arrive
	4. Repeat three times to get more RTT values for an average
	5. Calculate the average RTT
	6. Add half of RTT to get estamate travel time for the connection
	*/

	//Clear any reamining times
	this->m_ping_times.clear();

	for (int i = 0; i < 300; i++)
	{
		//Start the clock
		this->Clock_Start();

		//Send the packet
		this->Send(DefaultMessageIDTypes::R_CLOCK_SYNC, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);

		//Wait for the message until it arrives, When it does it will set the variable to false and end the loop
		while (this->m_ping_in_progress)
		{
			this->Update();
		}

	}

	this->m_Avg_Delay = this->GetAvrgRTT() / 2; //nano-seconds
	return this->m_Avg_Delay;
}
