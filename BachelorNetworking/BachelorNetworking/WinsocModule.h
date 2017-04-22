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
	SOCKET	m_ListnerSocket;
	SOCKET	m_ConnectionSocket;

	int			m_ClientID;
	int			m_PacketID;
	std::string	my_ip;

public:

private:
	bool	AcceptNewClient();
	void	ReadMessagesFromClients();
	int		GetMyIp();

public:
	WinsocModule();
	~WinsocModule();

	int		Initialize();
	int		Shutdown();
	void	Update();
	int		Connect(char* ip);
};

#endif;
