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

float RakNetModule::GetAvrgRTT()
{
	float result = 0;
	int count = 0;
	std::vector<int>::iterator itr;

	for (itr = this->m_ping_times.begin(); itr != this->m_ping_times.end();)
	{
		result += *itr._Ptr;
		count++;
	}

	result = result / count;

	return result;
}

RakNetModule::RakNetModule()
{
	this->peer = nullptr;
	this->transferComplete = false;
}

RakNetModule::~RakNetModule()
{
}

bool RakNetModule::Initialize()
{
	this->peer = RakNet::RakPeerInterface::GetInstance();
	this->listner = RakNet::SocketDescriptor(6881, 0);
	
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
	Packet p;
	
	for (RaKpacket = peer->Receive(); RaKpacket; peer->DeallocatePacket(RaKpacket), RaKpacket = peer->Receive())
	{
		int j = sizeof(RaKpacket->data);
		p.deserialize((char*)RaKpacket->data);

		switch (RaKpacket->data[0])
		{

		case R_CLOCK_SYNC:

			this->Send(DefaultMessageIDTypes::R_CLOCK_SYNC_RESPONSE, CLOCK_SYNC_RESPONSE, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
			break;

		case R_CLOCK_SYNC_RESPONSE:

			this->Clock_Stop();
			break;

		case R_CONNECTION_REQUEST:
			printf("Recived CONNECTION_REQUEST Packet");
			break;

		case R_TEST:
			printf("Recived Test Packet %d", p.packet_ID);
			this->Send(DefaultMessageIDTypes::R_TRANSFER_COMPLETE, TRANSFER_COMPLETE, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);
			break;

		case R_TRANSFER_COMPLETE :
			this->transferComplete = true;

			break;

		default:
			printf("Unkown packet type %d\n", p.packet_type);
		}
	}

}

bool RakNetModule::Connect(char * ip)
{
	
	this->peer->Startup(1, &this->listner, 1);
	this->peer->SetMaximumIncomingConnections(2);
	this->peer->Connect(ip, 6881, 0, 0);

	return false;
}

void RakNetModule::Send(DefaultMessageIDTypes id, PacketHeader headertype, PacketPriority priority, PacketReliability reliability)
{
	const unsigned int packet_size = sizeof(RakNetPacket);
	char packet_data[packet_size];

	RakNetPacket packet;
	packet.typeId = id;
	packet.packet_type = headertype;

	packet.serialize(packet_data);

	peer->Send(packet_data, packet_size, priority, reliability, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
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

	for (int i = 0; i < 3; i++)
	{
		//Start the clock
		this->Clock_Start();

		//Send the packet
		this->Send(DefaultMessageIDTypes::R_CLOCK_SYNC, CLOCK_SYNC, IMMEDIATE_PRIORITY, RELIABLE_SEQUENCED);

		//Wait for the message until it arrives, When it does it will set the variable to false and end the loop
		while (this->m_ping_in_progress)
		{
			this->Update();
		}

	}

	this->m_Avg_Delay = this->GetAvrgRTT() / 2; //nano-seconds
	return this->m_Avg_Delay;
}
