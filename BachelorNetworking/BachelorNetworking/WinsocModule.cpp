#include "WinsocModule.h"



WinsocModule::WinsocModule()
{
	this->m_ClientID = 0;
	this->m_PacketID = 0;

	this->m_CurrentProtocol = Protocol::NONE;
	this->m_ping_in_progress = false;
	this->m_Avg_Delay = 0;
	this->isConnected = false;
	this->tranferComplete = false;
	this->dataCounter = 0;
	this->m_currentID = 0;
	this->m_missedPackets = 0;
	this->network_data = new char[TCP_BUFFER_SIZE];
	this->UDP_network_data = new char[UDP_BUFFER_SIZE];
	this->network_message = new char[MESSAGE_BUFFER_SIZE];
	this->highest = -1;
	this->lowest = 9999999;
	currentIteration = 0;
}

WinsocModule::~WinsocModule()
{
	this->Shutdown();
}

int WinsocModule::Initialize(Protocol newProtocol, bool isSender)
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
		if (this->TCP_Initialize(false, isSender))
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
		if (this->TCP_Initialize(true, isSender))
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
		if (this->UDP_Initialize())
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

	if (this->m_TCP_SenderSocket != INVALID_SOCKET) {
		closesocket(this->m_TCP_SenderSocket);
		WSACleanup();
	}
	
	if (this->m_UDP_Socket != INVALID_SOCKET) {
		closesocket(this->m_UDP_Socket);
		WSACleanup();
	}

	delete[] this->network_data;
	delete[] this->UDP_network_data;
	delete[] this->network_message;

	this->network_data = nullptr;
	this->UDP_network_data = nullptr;
	this->network_message = nullptr;

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

	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	int	data_length;
	unsigned int header = -1;
	int data_read = 0;
	Packet p;
	DataPacket dp;
	
	// Setup timeval variable. If 0,0 it will return imedietly
	timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	// Setup fd_set structure
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(this->m_UDP_Socket, &fds);

	// Return value:
	// -1: error occurred
	// 0: timed out
	// > 0: data ready to be read
	int retval = select(this->m_UDP_Socket + 1, &fds, NULL, NULL, &timeout);
	if (retval == -1) {
		printf("Error, something went wrong with the 'select' function\n");
		return;
	}
	else if (retval == 0) {
		this->m_missedPackets++;
		printf("Timeout, No incoming data. Current Loss: %d\n", this->m_missedPackets);
		this->m_ping_in_progress = false;
		return;
	}
	
	//try to receive some data, this is a blocking call
	data_length = recvfrom(this->m_UDP_Socket, this->network_message, MESSAGE_BUFFER_SIZE, 0, (struct sockaddr *) &si_other, &slen);

	// If there was no data
	if (data_length <= 0)
	{
		//No data recieved, end the function
		return;
	}

	while (data_read != data_length)
	{
		//Read the header (skip the first 4 bytes since it is virtual function information)
		memcpy(&header, &this->network_message[data_read], sizeof(PacketHeader));

		switch (header)
		{

		case CLOCK_SYNC:
			//Resend a PING_RESPONSE
			this->UDP_Send(CLOCK_SYNC_RESPONSE, inet_ntoa(si_other.sin_addr));
			//printf("Recived CLOCK_SYNC Packet \n");
			data_read += sizeof(Packet);
			break;

		case CLOCK_SYNC_RESPONSE:

			this->Clock_Stop();
			//printf("Recived CLOCK_SYNC_RESPONSE Packet \n");
			data_read += sizeof(Packet);
			return;
			break;

		case CONNECTION_REQUEST:
			data_read += sizeof(Packet);

			//printf("Recived CONNECTION_REQUEST Packet \n");
			break;

		case DATA:
			memcpy(&dp, &this->network_message[data_read], sizeof(DataPacket));
			//printf("Recived DATA Packet %d of %d, Expected ", dp.ID, dp.nrOfPackets);
			//printf("%d \n", (this->dataCounter + 1));
			this->dataCounter++;
			if (this->dataCounter == dp.nrOfPackets)
			{
				this->UDP_Send(PacketHeader::TRANSFER_COMPLETE, inet_ntoa(si_other.sin_addr));
			}

			data_read += sizeof(DataPacket);

			break;

		case TRANSFER_COMPLETE:
			//printf("Recived TRANSFER_COMPLETE Packet \n");
			memcpy(&this->m_missedPackets, &this->network_message[data_read+4], sizeof(int));;
			data_read += 8;
			this->m_packet_loss.push_back(this->m_missedPackets);
			this->tranferComplete = true;
			this->dataCounter = 0;
			break;

		default:
			printf("Unkown packet type %d\n", header);
			data_read = data_length;
			break;
		}

	}

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
		int iResult = getaddrinfo(ip, RECIVER_PORT, &hints, &result);

		if (iResult != 0)
		{
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 0;
		}

		// Attempt to connect to an address until one succeeds
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

			// Set connectSocket to the host information
			this->m_TCP_SenderSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

			if (this->m_TCP_SenderSocket == INVALID_SOCKET) {
				printf("socket failed with error: %ld\n", WSAGetLastError());
				WSACleanup();
				return 0;
			}

			printf("Trying to connect to host...\n");
			iResult = SOCKET_ERROR;

			// Try to connect to host. This may take up to 20 seconds
			iResult = connect(this->m_TCP_SenderSocket, ptr->ai_addr, (int)ptr->ai_addrlen);

			if (iResult == SOCKET_ERROR)
			{
				closesocket(this->m_TCP_SenderSocket);
				this->m_TCP_SenderSocket = INVALID_SOCKET;
				printf("The host %s is down... did not connect\n", ip);
				return 0;
			}
		}

		// No longer need address info for server
		freeaddrinfo(result);

		// If connection failed
		if (this->m_TCP_SenderSocket == INVALID_SOCKET)
		{
			printf("Unable to connect to server!\n");
			WSACleanup();
			return 0;
		}

		// Set the mode of the socket to be nonblocking
		u_long iMode = 1;

		iResult = ioctlsocket(this->m_TCP_SenderSocket, FIONBIO, &iMode);
		if (iResult == SOCKET_ERROR)
		{
			printf("ioctlsocket failed with error: %d\n", WSAGetLastError());
			closesocket(this->m_TCP_SenderSocket);
			WSACleanup();
			return 0;
		}

		int value = OS_BUFFERS_SIZE;
		setsockopt(this->m_TCP_SenderSocket, SOL_SOCKET, SO_SNDBUF, (char*)value, sizeof(int) );	//TCP Options
		if (iResult == SOCKET_ERROR)
		{
			printf("incressing sender buffer failed with error: %d\n", WSAGetLastError());
			closesocket(this->m_TCP_SenderSocket);
			WSACleanup();
			return 0;
		}

		value = OS_BUFFERS_SIZE;
		iResult = setsockopt(this->m_TCP_SenderSocket, SOL_SOCKET, SO_RCVBUF, (char*)value, sizeof(int));
		if (iResult == SOCKET_ERROR) {
			printf("incressing reciver buffer failed with error: %d\n", WSAGetLastError());
			closesocket(this->m_UDP_Socket);
			WSACleanup();
			return 0;
		}

		//Send CONNECTION_REQUEST package
		const unsigned int packet_size = sizeof(Packet);

		Packet packet;
		packet.packet_type = CONNECTION_REQUEST;

		// Send the packet directly to the host
		NetworkService::sendMessage(this->m_TCP_SenderSocket, reinterpret_cast<char*>(&packet), packet_size);
		printf("Sent CONNECTION_REQUEST to host\n");

		// Add the host to connectedClients before getting a CONNECTION_ACCEPTED back 
		// since we need to know which client to listen for
		
		this->m_ClientID++;
		this->isConnected = true;

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
		int value = OS_BUFFERS_SIZE;

		setsockopt(otherClientSocket, SOL_SOCKET, SO_SNDBUF, (char*)value, sizeof(int));	//TCP Options
		setsockopt(otherClientSocket, SOL_SOCKET, SO_RCVBUF, (char*)value, sizeof(int));

		this->m_TCP_ConenctedSocket = otherClientSocket;
		this->m_TCP_SenderSocket = otherClientSocket;

		printf("client %d has been connected to the server\n", this->m_ClientID);
		this->m_ClientID++;
		this->isConnected = true;

		return true;
	}

	return false;
}

void WinsocModule::ReadMessagesFromClients()
{

		// The data buffer that will hold the incoming data
	unsigned int header = -1;			// The header variable that will hold the loaded header
	int old = -1;
	// The objects to load the data into
	Packet p;
	DataPacket dp;

	//Check if there is data
	int data_length = NetworkService::receiveMessage(this->m_TCP_SenderSocket, this->network_message, MESSAGE_BUFFER_SIZE);
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
		//memcpy(&header, &this->network_data[data_read], sizeof(PacketHeader));
		header = this->network_message[data_read];
		switch (header)
		{
			
		case CLOCK_SYNC :
			//Resend a PING_RESPONSE
			this->TCP_Send(CLOCK_SYNC_RESPONSE);
			//printf("Recived CLOCK_SYNC Packet \n");
			data_read += sizeof(Packet);
			break;

		case CLOCK_SYNC_RESPONSE:
			
			this->Clock_Stop();
			//printf("Recived CLOCK_SYNC_RESPONSE Packet \n");
			data_read += sizeof(Packet);
			break;

		case CONNECTION_REQUEST :
			data_read += sizeof(Packet);

			//printf("Recived CONNECTION_REQUEST Packet \n");
			break;

		case DATA:
			old = data_read;
			memcpy(&dp, &network_data[data_read], sizeof(DataPacket));
			//printf("Recived DATA Packet %d of %d, Expected ", dp.ID, dp.nrOfPackets);
			//printf("%d \n",(this->dataCounter + 1) );
			this->dataCounter++;
			if (this->dataCounter == dp.nrOfPackets)
			{
				this->TCP_Send(PacketHeader::TRANSFER_COMPLETE);
			}
			
			data_read += sizeof(DataPacket);
			
			break;

		case TRANSFER_COMPLETE :
			//printf("Recived TRANSFER_COMPLETE Packet \n");
			data_read += sizeof(Packet);
			this->tranferComplete = true;
			this->dataCounter = 0;
			break;

		default:
			this->dataCounter++;
			printf("Unkown packet type %d, %d\n", header, this->dataCounter);
			data_read = data_length;
			break;
		}

	}

}

void WinsocModule::TCP_WaitForData()
{
	int data_length = NetworkService::receiveMessage(this->m_TCP_SenderSocket, this->network_data, TCP_BUFFER_SIZE);
	int data_read = 0;

	// If there was no data
	if (data_length <= 0)
	{
		//No data recieved, end the function
		return;
	}

	this->data_total += data_length;
	printf("%d\n",this->data_total);
	
	if (data_total >= DATA_SIZE)
	{
		this->TCP_Send(TRANSFER_COMPLETE);
		printf("Sent TRANSFER_COMPLETE\n");
		data_total = 0;
	}
}

void WinsocModule::UDP_WaitForData(int packetSize)
{
	int data_length = -1;
	int data_read = 0;

	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(this->m_UDP_Socket, &fds);

	int retval = select(this->m_UDP_Socket + 1, &fds, NULL, NULL, &timeout);
	if (retval == -1) {
		printf("Error, something went wrong with the 'select' function\n");
	}
	else if (retval == 0) {
		//printf("Timeout, No incoming data");
		return;
	}

	//try to receive some data, this is a blocking call
	if ((data_length = recvfrom(this->m_UDP_Socket, this->UDP_network_data, UDP_BUFFER_SIZE, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{
		printf("recvfrom() failed with error code : %d \n", WSAGetLastError());
		//exit(EXIT_FAILURE);
	}

	int id;
	int iteration;
	memcpy(&id,UDP_network_data,sizeof(int));
	memcpy(&iteration, UDP_network_data+4, sizeof(int));

	if (iteration != currentIteration)
	{
		return;
	}

	if (id == 1) //If we recive a new set of packages
	{
		//Reset the variables
		this->m_currentID = 0;	
		this->data_total = 0;
		this->m_missedPackets = 0;
	}

	int dif = id - this->m_currentID;

	if (dif > 1)
	{
		this->m_missedPackets += dif-1;
	}

	this->m_currentID = id;

	this->data_total += data_length;
	//printf("%d, Loss: %d\n", this->data_total, this->m_missedPackets);

	if (data_total >= DATA_SIZE)
	{
		//this->UDP_Send(TRANSFER_COMPLETE, inet_ntoa(si_other.sin_addr));
		
		this->m_RecvAddr.sin_addr.s_addr = inet_addr(inet_ntoa(si_other.sin_addr));
		const unsigned int packet_size = sizeof(Packet);

		char data[12];
		int value = (int)TRANSFER_COMPLETE;
		memcpy(&data, &value, sizeof(int));
		double loss = (double)(this->m_missedPackets / (double)(this->data_total / packetSize));
		memcpy(&data[4], &loss, sizeof(double));
		
		sendto(this->m_UDP_Socket, reinterpret_cast<char*>(&data), sizeof(data), 0, (struct sockaddr*) &this->m_RecvAddr, sizeof(this->m_RecvAddr));

		printf("Sent TRANSFER_COMPLETE, Packages Recived: %d, Loss: %d\n", this->data_total/ packetSize, this->m_missedPackets);
		this->currentIteration++;
		this->data_total = 0;
		this->m_missedPackets = 0;
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

void WinsocModule::TCP_Send(PacketHeader headertype)
{
	const unsigned int packet_size = sizeof(Packet);

	Packet packet;
	packet.packet_type = headertype;

	NetworkService::sendMessage(this->m_TCP_SenderSocket, reinterpret_cast<char*>(&packet), packet_size);
}

int WinsocModule::TCP_Send_Data(int packetSize)
{
	char* data = new char[packetSize];

	//char data[TCP_PACKET_SIZE];
	//const unsigned int packet_size = sizeof(data);
	int nrOfPackets = ceil(DATA_SIZE / packetSize)+1;
	
	this->m_start_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();

	for(int i = 1; i <= nrOfPackets; i++)
	{
		NetworkService::sendMessage(this->m_TCP_SenderSocket, data, packetSize);
		
		//printf("Sent DataPacket %d\n", i);
	}

	//Recive Last ack
	while (this->tranferComplete == false)
	{
		this->TCP_Update();
	}

	auto end_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();

	int result = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - this->m_start_time).count();;

	this->tranferComplete = false;

	delete[] data;
	data = nullptr;

	return result;
}

void WinsocModule::UDP_Send(PacketHeader headertype, char* ip)
{

	this->m_RecvAddr.sin_addr.s_addr = inet_addr(ip);
	const unsigned int packet_size = sizeof(Packet);

	Packet packet;
	packet.packet_type = headertype;

	sendto(this->m_UDP_Socket, reinterpret_cast<char*>(&packet), packet_size, 0, (struct sockaddr*) &this->m_RecvAddr, sizeof(this->m_RecvAddr));

	
}

int WinsocModule::UDP_Send_Data(char * ip, int iteration, int packetSize)
{

	this->m_RecvAddr.sin_addr.s_addr = inet_addr(ip);
	//Se if we recived a response
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	int	data_length;
	unsigned int header = -1;
	int data_read = 0;
	double loss;

	// Setup timeval variable. If 0,0 it will return imedietly
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	char data[61440];
	//char data[UDP_PACKET_SIZE];
	//const unsigned int packet_size = sizeof(*data);
	int id = 0;
	sockaddr* add = (struct sockaddr*) &this->m_RecvAddr;
	int addS = sizeof(this->m_RecvAddr);

	this->m_start_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();
	
	while(this->tranferComplete != true)
	{	
		id++;
		memcpy(&data, &id, sizeof(int));
		memcpy(&data[4], &iteration, sizeof(int));	//Which iteration the packet is from
		if (sendto(this->m_UDP_Socket, data, packetSize, 0, (struct sockaddr*) &this->m_RecvAddr, sizeof(this->m_RecvAddr)) == SOCKET_ERROR)
		{
			printf("send failed\n");
		}
		else 
		{
			//printf("Sent DataPacket %d\n", id);
		}

		// Setup fd_set structure
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(this->m_UDP_Socket, &fds);

		// Return value:
		// -1: error occurred
		// 0: timed out
		// > 0: data ready to be read
		int retval = select(this->m_UDP_Socket + 1, &fds, NULL, NULL, &timeout);
		if (retval == -1) {
			printf("Error, something went wrong with the 'select' function\n");
		}
		else if (retval == 0) {
			//printf("Timeout, No incoming data. Current Loss: %d\n", this->m_missedPackets);
		}
		else //There where data
		{
			//try to receive some data, this is a blocking call
			data_length = recvfrom(this->m_UDP_Socket, this->network_message, MESSAGE_BUFFER_SIZE, 0, (struct sockaddr *) &si_other, &slen);

			while (data_read != data_length)
			{
				//Read the header (skip the first 4 bytes since it is virtual function information)
				memcpy(&header, &this->network_message[data_read], sizeof(PacketHeader));

				switch (header)
				{
				case TRANSFER_COMPLETE:
					//printf("Recived TRANSFER_COMPLETE Packet \n");
					memcpy(&loss, &this->network_message[data_read + 4], sizeof(double));;
					
					this->m_packet_loss.push_back(loss);
					this->tranferComplete = true;
					data_read += 12;
					this->dataCounter = 0;
					break;

				default:
					printf("Unkown packet type %d\n", header);
					data_read = data_length;
					break;
				}

			}
		}

	}
	auto end_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();

	int result = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - this->m_start_time).count();;

	this->tranferComplete = false;

	return result;
}

float WinsocModule::GetAvrgRTT()
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

int WinsocModule::Calculate_AVG_Delay(int packetsize, int pingIterations)
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
	this->m_missedPackets = 0;
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
	
	//Add the header at the begining of the data
	int value = (int)CLOCK_SYNC;
	memcpy(data, &value, sizeof(int));

	for (int i = 0; i < pingIterations; i++)
	{
		// Set current time
		this->m_start_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();
		// Set to not send more pings since it will disrupt the timers
		this->m_ping_in_progress = true;

		//Send the packet
		NetworkService::sendMessage(this->m_TCP_SenderSocket, data, sizeof(data));

		//Wait for the message until it arrives, When it does it will set the variable to false and end the loop
		while (this->m_ping_in_progress)
		{
			this->ReadMessagesFromClients();
		}

	}

	float total = 0;
	int count = 0;
	std::vector<int>::iterator itr;
	std::ofstream file;
	std::ostringstream os;

	os << "../Logs/" << packetsize << " " << pingIterations << ".txt";
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

int WinsocModule::Calculate_AVG_Delay(char * ip, int packetsize, int pingIterations)
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

	this->m_RecvAddr.sin_addr.s_addr = inet_addr(ip);
	this->m_ping_times.clear();
	this->highest = -1;
	this->lowest = 9999999;
	this->m_missedPackets = 0;
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

	//Add the header at the begining of the data
	int value = (int)CLOCK_SYNC;
	memcpy(data, &value, sizeof(int));

	for (int i = 0; i < pingIterations; i++)
	{
		// Set current time
		this->m_start_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();

		// Set to not send more pings since it will disrupt the timers
		this->m_ping_in_progress = true;
		
		//Send the packet
		//this->UDP_Send(CLOCK_SYNC, ip);

		sendto(this->m_UDP_Socket, data, sizeof(data), 0, (struct sockaddr*) &this->m_RecvAddr, sizeof(this->m_RecvAddr));

		while (this->m_ping_in_progress)
		{
			this->UDP_Update();
		}

	}


	float total = 0;
	int count = 0;
	std::vector<int>::iterator itr;
	std::ofstream file;
	std::ostringstream os;

	os << "../Logs/" << packetsize << " " << pingIterations << ".txt";
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

void WinsocModule::Clear_PacketLoss_Vector()
{
	this->m_packet_loss.clear();
}

void WinsocModule::Calcualet_Loss()
{
	float result = 0;
	int count = 0;
	std::vector<double>::iterator itr;
	this->highestLoss = -1;
	this->lowestLoss = 9999999;

	for (itr = this->m_packet_loss.begin(); itr != this->m_packet_loss.end();)
	{
		double value = (double)*itr._Ptr;
		
		if (value > this->highestLoss)
		{
			this->highestLoss = value;
		}
		
		if (value < lowestLoss)
		{
			this->lowestLoss = value;
		}

		result += value;
		count++;
		itr++;
	}

	this->averageLoss = (double)result / count;

}

bool WinsocModule::GetIsConnected()
{
	return this->isConnected;
}

bool WinsocModule::GetTransferComplete()
{
	return this->tranferComplete;
}

int WinsocModule::TCP_Initialize(bool noDelay, bool isSender)
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
	if (isSender)
	{
		iResult = getaddrinfo(NULL, SENDER_PORT, &hints, &result);	//NULL = Dont need addres since it will be on local machine
	}
	else
	{
		iResult = getaddrinfo(NULL, RECIVER_PORT, &hints, &result);	//NULL = Dont need addres since it will be on local machine
	}

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

int WinsocModule::UDP_Initialize()
{
	sockaddr_in local;
	int iResult;

	this->GetMyIp();//Set the local ip

	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = 6881; // choose any

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

	//int value = OS_BUFFERS;
	//iResult = setsockopt(this->m_UDP_Socket, SOL_SOCKET, SO_RCVBUF, (char*)value, sizeof(int));
	//if (iResult == SOCKET_ERROR) {
	//	printf("incressing reciver buffer failed with error: %d\n", WSAGetLastError());
	//	closesocket(this->m_UDP_Socket);
	//	WSACleanup();
	//	return 0;
	//}

	//value = OS_BUFFERS;	// Set the value again if we want to change it
	//setsockopt(this->m_UDP_Socket, SOL_SOCKET, SO_SNDBUF, (char*)value, sizeof(int));
	//if (iResult == SOCKET_ERROR) {
	//	printf("incressing sender buffer failed with error: %d\n", WSAGetLastError());
	//	closesocket(this->m_UDP_Socket);
	//	WSACleanup();
	//	return 0;
	//}

	this->m_RecvAddr.sin_family = AF_INET;
	this->m_RecvAddr.sin_port = 6881;

	return 1;
}

void WinsocModule::Clock_Start()
{
	// Set current time
	this->m_start_time = std::chrono::time_point<std::chrono::steady_clock>::clock::now();

	// Set to not send more pings since it will disrupt the timers
	this->m_ping_in_progress = true;
}

int WinsocModule::Clock_Stop(bool ms)
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

int WinsocModule::GetHighest()
{
	return this->highest;
}

int WinsocModule::GetLowest()
{
	return this->lowest;
}

int WinsocModule::GetLost()
{
	return this->m_missedPackets;
}

double WinsocModule::GetAverageLoss()
{
	return this->averageLoss;
}

double WinsocModule::GetHighestLoss()
{
	return this->highestLoss;
}

double WinsocModule::GetLowestLoss()
{
	return this->lowestLoss;
}



