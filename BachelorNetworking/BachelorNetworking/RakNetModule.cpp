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
}

RakNetModule::~RakNetModule()
{
}

bool RakNetModule::Initialize()
{
	this->peer = RakNet::RakPeerInterface::GetInstance();
	this->listner = RakNet::SocketDescriptor((int)DEFAULT_PORT, 0);
	
	return false;
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
		p.deserialize((char*)RaKpacket->data);

		switch (p.packet_type)
		{

		case CLOCK_SYNC:

			this->Send(CLOCK_SYNC_RESPONSE, IMMEDIATE_PRIORITY, UNRELIABLE);

		case CLOCK_SYNC_RESPONSE:

			this->Clock_Stop();

		case CONNECTION_REQUEST:
			printf("Recived CONNECTION_REQUEST Packet");

		case TEST:
			printf("Recived Test Packet %d", p.packet_ID);


		default:
			printf("Unkown packet type %d\n", p.packet_type);
		}
	}

}

bool RakNetModule::Connect(char * ip)
{
	
	this->peer->Startup(1, &this->listner, 1);
	this->peer->SetMaximumIncomingConnections(2);

	return false;
}

void RakNetModule::Send(PacketHeader headertype, PacketPriority priority, PacketReliability reliability)
{
	const unsigned int packet_size = sizeof(Packet);
	char packet_data[packet_size];

	Packet packet;
	packet.packet_type = headertype;

	packet.serialize(packet_data);

	peer->Send(packet_data, packet_size, priority, reliability, 0, RakNet::UNASSIGNED_RAKNET_GUID, true);
}

void RakNetModule::Calculate_AVG_Delay()
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
		this->Send(CLOCK_SYNC);

		//Wait for the message until it arrives, When it does it will set the variable to false and end the loop
		while (this->m_ping_in_progress)
		{
			this->Update();
		}

	}

	this->m_Avg_Delay = this->GetAvrgRTT() / 2; //nano-seconds
}
