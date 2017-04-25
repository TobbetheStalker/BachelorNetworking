#include "WinsocModule.h"



WinsocModule::WinsocModule()
{
	this->m_ClientID = 0;
	this->m_PacketID = 0;

	this->m_CurrentProtocol = Protocol::NONE;
}

WinsocModule::~WinsocModule()
{
}

int WinsocModule::Initialize(Protocol newProtocol)
{
	printf("Initializing Network module... \n");

	WSADATA wsaData;

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 0;
	}

	//Chose what protocol to initialize the sockets with
	if (newProtocol == Protocol::TCP)
	{
		if (this->Initialize_TCP(false))
		{
			//Success
			this->m_CurrentProtocol = newProtocol;
			printf("Network module Initialized with type %d protocol\n", this->m_CurrentProtocol);
		}
		else 
		{
			//Error
			printf("Network module Initialization FAILED with type %d protocol\n", this->m_CurrentProtocol);
		}
	}
	else if (newProtocol == Protocol::TCP_WITH_NODELAY)
	{
		if (this->Initialize_TCP(true))
		{
			this->m_CurrentProtocol = newProtocol;
			printf("Network module Initialized with type %d protocol\n", this->m_CurrentProtocol);
		}
		else
		{
			printf("Network module Initialization FAILED with type %d protocol\n", this->m_CurrentProtocol);
		}
	}
	else if (newProtocol == Protocol::UDP)
	{
		if (this->Initialize_UDP())
		{
			this->m_CurrentProtocol = newProtocol;
			printf("Network module Initialized with type %d protocol\n", this->m_CurrentProtocol);
		}
		else
		{
			printf("Network module Initialization FAILED with type %d protocol\n", this->m_CurrentProtocol);
		}
	}

	return 1;
}

int WinsocModule::Shutdown()
{
	if (this->m_TCP_ListnerSocket != INVALID_SOCKET) {
		closesocket(this->m_TCP_ListnerSocket);
		WSACleanup();
	}

	if (this->m_TCP_ConnectionSocket != INVALID_SOCKET) {
		closesocket(this->m_TCP_ConnectionSocket);
		WSACleanup();
	}
	
	this->m_CurrentProtocol = Protocol::NONE;
	return 0;
}

void WinsocModule::Update()
{
	if (this->m_CurrentProtocol == Protocol::UDP)
	{
		this->UDP_Update();
	}
	else
	{
		this->TCP_Update();
	}
}

void WinsocModule::TCP_Update()
{
	this->AcceptNewClient();				// Get any new clients
	this->ReadMessagesFromClients();		//Read messages
}

void WinsocModule::UDP_Update()
{
}

int WinsocModule::TCP_Connect(char * ip)
{
	addrinfo *result = NULL;
	addrinfo *ptr = NULL;
	addrinfo hints;

	//if (this->m_IP == ip)	//if my_ip is the same as the ip we try to connect to
	//{
	//	printf("Cannot connect to %s as it is this machines local ip\n", ip);
	//	return 0;
	//}
	//else
	//{
		ZeroMemory(&hints, sizeof(hints));	// Empties hint		
		hints.ai_family = AF_INET;			// The Internet Protocol version 4 (IPv4) address family
		hints.ai_socktype = SOCK_STREAM;	// Provides sequenced, reliable, two-way, connection-based byte streams with an OOB data transmission mechanism
		hints.ai_protocol = IPPROTO_TCP;    // Set to use TCP

		// Resolve the server address and port
		int iResult = getaddrinfo(ip, DEFAULT_PORT, &hints, &result);

		if (iResult != 0)
		{
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 0;
		}

		// Attempt to connect to an address until one succeeds
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

			// Set connectSocket to the host information
			this->m_TCP_ConnectionSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

			if (this->m_TCP_ConnectionSocket == INVALID_SOCKET) {
				printf("socket failed with error: %ld\n", WSAGetLastError());
				WSACleanup();
				return 0;
			}

			printf("Trying to connect to host...\n");
			iResult = SOCKET_ERROR;

			// Try to connect to host. This may take up to 20 seconds
			iResult = connect(this->m_TCP_ConnectionSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

			if (iResult == SOCKET_ERROR)
			{
				closesocket(this->m_TCP_ConnectionSocket);
				this->m_TCP_ConnectionSocket = INVALID_SOCKET;
				printf("The host %s is down... did not connect\n", ip);
				return 0;
			}
		}

		// No longer need address info for server
		freeaddrinfo(result);

		// If connection failed
		if (this->m_TCP_ConnectionSocket == INVALID_SOCKET)
		{
			printf("Unable to connect to server!\n");
			WSACleanup();
			return 0;
		}

		// Set the mode of the socket to be nonblocking
		u_long iMode = 1;

		iResult = ioctlsocket(this->m_TCP_ConnectionSocket, FIONBIO, &iMode);
		if (iResult == SOCKET_ERROR)
		{
			printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
			closesocket(this->m_TCP_ConnectionSocket);
			WSACleanup();
			return 0;
		}

		//Send CONNECTION_REQUEST package
		const unsigned int packet_size = sizeof(Packet);
		char packet_data[packet_size];

		Packet packet;
		packet.packet_type = CONNECTION_REQUEST;
		packet.packet_ID = this->m_PacketID;
		this->m_PacketID++;

		packet.serialize(packet_data);

		// Send the packet directly to the host
		NetworkService::sendMessage(this->m_TCP_ConnectionSocket, packet_data, packet_size);
		printf("Sent CONNECTION_REQUEST to host\n");

		// Add the host to connectedClients before getting a CONNECTION_ACCEPTED back 
		// since we need to know which client to listen for
		this->m_TCP_ListnerSocket = this->m_TCP_ConnectionSocket;
		printf("Listner socket has been set to listen to same address as the Connection socket \n");
		this->m_ClientID++;

		return 1;
	//}

}

bool WinsocModule::AcceptNewClient()
{
	SOCKET otherClientSocket;
	// If client waiting, accept the connection and save the socket
	otherClientSocket = accept(this->m_TCP_ListnerSocket, NULL, NULL);

	if (otherClientSocket != INVALID_SOCKET)
	{

		if (this->m_CurrentProtocol == Protocol::TCP_WITH_NODELAY)
		{
			// Disable the nagle effect on the client's socket
			char value = 1;
			setsockopt(otherClientSocket, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));	//TCP Options
		}

		this->m_TCP_ConnectionSocket = otherClientSocket;
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
	int data_length = NetworkService::receiveMessage(this->m_TCP_ListnerSocket, network_data, MAX_PACKET_SIZE);
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
			
		case CONNECTION_REQUEST :
			p.deserialize(&network_data[data_read]);
			data_read += sizeof(Packet);

			printf("Recived CONNECTION_REQUEST Packet");
			this->SendPacket(PacketHeader::TEST);

		case TEST :
			p.deserialize(&network_data[data_read]);
			data_read += sizeof(Packet);
			
			printf("Recived Test Packet %d", p.packet_ID);
			this->SendPacket(PacketHeader::TEST);

			

		default:
			printf("Unkown packet type %d\n", header);
			data_read = data_length;	//Break
		}

	}

}

int WinsocModule::GetMyIp()
{
	char ac[80];
	struct in_addr addr;

	if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR)
	{
		printf("failed to get local ip");
		0;
	}

	struct hostent *phe = gethostbyname(ac);
	if (phe == 0) {
		printf("failed to get local ip");
		0;
	}

	for (int i = 0; phe->h_addr_list[i] != 0; ++i) {

		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
	}
	unsigned char b1 = addr.S_un.S_un_b.s_b1;
	unsigned char b2 = addr.S_un.S_un_b.s_b2;
	unsigned char b3 = addr.S_un.S_un_b.s_b3;
	unsigned char b4 = addr.S_un.S_un_b.s_b4;

	// Stream the data as int into the string
	std::stringstream ss;
	ss << (int)b1 << "." << (int)b2 << "." << (int)b3 << "." << (int)b4;

	this->m_IP.append(ss.str());	// Set my_ip to the local ip-address of the machine

	return 1;
}

void WinsocModule::SendPacket(PacketHeader headertype)
{
	const unsigned int packet_size = sizeof(Packet);
	char packet_data[packet_size];

	Packet packet;
	packet.packet_type = headertype;
	packet.packet_ID = this->m_PacketID++;

	packet.serialize(packet_data);
	
	NetworkService::sendMessage(this->m_TCP_ConnectionSocket, packet_data, packet_size);
}

int WinsocModule::Initialize_TCP(bool noDelay)
{
	this->m_TCP_ListnerSocket = INVALID_SOCKET;	// The socket that will listen
	int iResult;

	// Address info for the listenSocket to listen to
	struct addrinfo *result = NULL;
	struct addrinfo hints;

	// Set address information
	ZeroMemory(&hints, sizeof(hints));	// Empties hints
	hints.ai_family = AF_INET;			// The Internet Protocol version 4 (IPv4) address family
	hints.ai_socktype = SOCK_STREAM;	// Provides sequenced, reliable, two-way, connection-based byte streams with an OOB data transmission mechanism
	hints.ai_protocol = IPPROTO_TCP;    // Set to use TCP
	hints.ai_flags = AI_PASSIVE;		// The socket address will be used in a call to the bind function
	
	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);	//NULL = Dont need addres since it will be on local machine

	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 0;
	}

	// Create a SOCKET for connecting to server
	this->m_TCP_ListnerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (this->m_TCP_ListnerSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 0;
	}

	// Set the mode of the socket to be nonblocking
	u_long iMode = 1;
	iResult = ioctlsocket(this->m_TCP_ListnerSocket, FIONBIO, &iMode);

	if (iResult == SOCKET_ERROR) {
		printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
		closesocket(this->m_TCP_ListnerSocket);
		WSACleanup();
		return 0;
	}

	// Setup the listening socket
	iResult = bind(this->m_TCP_ListnerSocket, result->ai_addr, (int)result->ai_addrlen);

	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(this->m_TCP_ListnerSocket);
		WSACleanup();
		return 0;
	}

	// No longer need address information
	freeaddrinfo(result);


	// Start listening for new clients attempting to connect
	iResult = listen(this->m_TCP_ListnerSocket, SOMAXCONN);

	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(this->m_TCP_ListnerSocket);
		WSACleanup();
		return 0;
	}

	this->GetMyIp();

	return 1;
}

int WinsocModule::Initialize_UDP()
{
	sockaddr_in local;
	int iResult;

	this->GetMyIp();//Set the local ip

	local.sin_family = AF_INET;
	local.sin_addr.s_addr = inet_addr(this->m_IP.c_str());
	local.sin_port = (int)DEFAULT_PORT; // choose any

	// create the socket
	this->m_UDP_Socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	// bind to the local address

	iResult = bind(this->m_UDP_Socket, (sockaddr *)&local, sizeof(local));

	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(this->m_UDP_Socket);
		WSACleanup();
		return 0;
	}

	return 1;
}
