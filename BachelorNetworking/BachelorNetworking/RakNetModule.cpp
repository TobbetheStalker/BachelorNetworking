#include "RakNetModule.h"

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
