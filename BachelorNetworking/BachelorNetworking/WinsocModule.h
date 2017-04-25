#ifndef WINSOCMODULE_H
#define WINSOCMODULE_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <sstream>
#include <ws2tcpip.h>
#include "NetworkData.h"
#include "NetworkService.h"

#ifdef _WIN64
#define PACKETOFFSET 8
#else
#define PACKETOFFSET 4
#endif

#define DEFAULT_PORT "6881"

class WinsocModule
{

private:
	SOCKET	m_TCP_ListnerSocket;
	SOCKET	m_TCP_ConnectionSocket;
	SOCKET	m_UDP_Socket;

	int			m_ClientID;
	int			m_PacketID;
	std::string	m_IP;

	Protocol m_CurrentProtocol;

public:

private:
	bool	AcceptNewClient();
	void	ReadMessagesFromClients();
	int		GetMyIp();

	int		TCP_Initialize(bool noDelay);
	int		UDP_Initialize();

public:
	WinsocModule();
	~WinsocModule();

	int		Initialize(Protocol newProtocol);
	int		Shutdown();
	void	Update();
	void	TCP_Update();
	void	UDP_Update();
	int		TCP_Connect(char* ip);

	void	TCP_Send(PacketHeader headertype);
	void	UDP_Send(PacketHeader headertype);
};

#endif;
