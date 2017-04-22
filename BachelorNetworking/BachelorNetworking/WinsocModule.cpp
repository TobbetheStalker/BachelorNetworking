#include "WinsocModule.h"



WinsocModule::WinsocModule()
{
}

WinsocModule::~WinsocModule()
{
}

int WinsocModule::Initialize()
{
	return 0;
}

int WinsocModule::Shudown()
{
	return 0;
}

void WinsocModule::Update()
{
	this->AcceptNewClient();				// Get any new clients
	this->ReadMessagesFromClients();		//Read messages
}

int WinsocModule::Connect(char * ip)
{
	return 0;
}

bool WinsocModule::AcceptNewClient()
{
	SOCKET otherClientSocket;
	// If client waiting, accept the connection and save the socket
	otherClientSocket = accept(this->m_ListnerSocket, NULL, NULL);

	if (otherClientSocket != INVALID_SOCKET)
	{
		// Disable the nagle effect on the client's socket
		char value = 1;
		setsockopt(otherClientSocket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));	//TCP Options

		this->m_ConnectionSocket = otherClientSocket;																		// Insert new client into session id table 
		printf("client %d has been connected to the server\n", this->m_ClientID);
		this->m_ClientID++;

		return true;
	}

	return false;
}

void WinsocModule::ReadMessagesFromClients()
{

	char network_data[MAX_PACKET_SIZE];	// The data buffer that will hold the incoming data
	unsigned int header = -1;			// The header variable that will hold the loaded header

										// The objects to load the data into
	Packet p;

	//Check if there is data
	int data_length = NetworkService::receiveMessage(this->m_ListnerSocket, network_data, MAX_PACKET_SIZE);
	int data_read = 0;

	// If there was no data
	if (data_length <= 0)
	{
		//No data recieved, end the function
		return;
	}
	
	while (data_read != data_length)
	{
		//Read the header (skip the first 4 bytes since it is virtual function information)
		memcpy(&header, &network_data[data_read + PACKETOFFSET], sizeof(PacketHeader));

		switch (header)
		{

			default:
				printf("Unkown packet type %d\n", header);
				data_read = data_length;	//Break
		}

	}

}
