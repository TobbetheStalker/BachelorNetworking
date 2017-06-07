#include "RakNetModule.h"

void RakNetModule::Clock_Start()
{
	// Set current time
	this->m_start_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();

	// Set to not send more pings since it will disrupt the timers
	this->m_ping_in_progress = true;
}

int RakNetModule::Clock_Stop(bool ms)
{
	int result = 0;

	//Get the end time
	auto end_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();

	//Calculate the delta time togheter with end and start time to nano-seconds
	if (ms)
	{
		result = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - this->m_start_time).count();
	}
	else
	{
		result = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - this->m_start_time).count();
	}

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

int RakNetModule::GetHighest()
{
	return this->highest;
}

int RakNetModule::GetLowest()
{
	return this->lowest;
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
	this->data_total = 0;
	this->highest = -1;
	this->lowest = 9999999;
}

RakNetModule::~RakNetModule()
{
}

bool RakNetModule::Initialize()
{
	this->peer = RakNet::RakPeerInterface::GetInstance();
	this->socketDescriptor = RakNet::SocketDescriptor(6881, 0);
	socketDescriptor.socketFamily = AF_INET;
	
	this->peer->Startup(3, &this->socketDescriptor, 1);
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
			//printf("Recived R_CLOCK_SYNC Packet\n");
			this->Send(DefaultMessageIDTypes::R_CLOCK_SYNC_RESPONSE, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
			break;

		case R_CLOCK_SYNC_RESPONSE:
			//printf("Recived R_CLOCK_SYNC_RESPONSE Packet\n");
			this->Clock_Stop();
			break;

		case R_CONNECTION_REQUEST:
			printf("Recived R_CONNECTION_REQUEST Packet\n");
			break;

		case R_DATA:

			printf("Recived R_DATA Packet\n");
			break;

		case R_TRANSFER_COMPLETE :
			printf("Recived R_TRANSFER_COMPLETE Packet\n");
			this->transferComplete = true;
			return;
			break;

		default:
			printf("Unkown packet type %d\n", RaKpacket->data[0]);
			break;
		}
	}

}

void RakNetModule::WaitForData()
{
	RakNet::Packet* RaKpacket;

	for (RaKpacket = peer->Receive(); RaKpacket; peer->DeallocatePacket(RaKpacket), RaKpacket = peer->Receive())
	{

		if (RaKpacket != nullptr)
		{
			this->data_total += sizeof(RaKpacket);

			if (this->data_total >= DATA_SIZE)
			{
				this->Send(R_TRANSFER_COMPLETE, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
				this->data_total = 0;
			}
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
	int nrOfPackets = ceil(DATA_SIZE / (sizeof(RakNetDataPacket)));
	const unsigned int packet_size = sizeof(RakNetDataPacket);
	
	for (int i = 1; i <= nrOfPackets; i++)
	{
		int j = peer->Send(reinterpret_cast<char*>(&packet), sizeof(packet), HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);

		printf("Sent DataPacket %d\n", i);
	}

}

int RakNetModule::Calculate_AVG_Delay(int packetsize)
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
	this->highest = -1;
	this->lowest = 9999999;
	char* data = nullptr;

	switch (packetsize)
	{
	case 4:
		data = new char[4];
		break;
	case 512:
		data = new char[512];
		break;
	case 1024:
		data = new char[1024];
		break;
	case 1500:
		data = new char[1500];
		break;
	case 2048:
		data = new char[2048];
		break;
	}
	printf("Packetsize: %d\n", packetsize);


	data[0] = DefaultMessageIDTypes::R_CLOCK_SYNC;
	for (int i = 0; i < PING_ITERATIONS; i++)
	{
		//Start the clock
		//this->Clock_Start();
		this->m_start_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();
		this->m_ping_in_progress = true;

		//Send the packet
		//this->Send(DefaultMessageIDTypes::R_CLOCK_SYNC, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
		peer->Send(data, sizeof(data), IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);

		//Wait for the message until it arrives, When it does it will set the variable to false and end the loop
		while (this->m_ping_in_progress)
		{
			this->Update();
		}

	}

	float total = 0;
	int count = 0;
	std::vector<int>::iterator itr;
	std::ofstream file;
	std::ostringstream os;

	os <<"../Logs/" << packetsize << " " << PING_ITERATIONS << ".txt";
	file.open(os.str());
	for (itr = this->m_ping_times.begin(); itr != this->m_ping_times.end();)
	{
		int value = *itr._Ptr / 2; //We only care of one-way time

		if (value > this->highest)
		{
			this->highest = value;
		}

		if (value < this->lowest)
		{
			this->lowest = value;
		}
		file << value << "\n";
		total += value;
		count++;
		itr++;
	}

	file.close();
	this->m_Avg_Delay = (total / count); //nano-seconds

	delete[] data;

	return this->m_Avg_Delay;
}
