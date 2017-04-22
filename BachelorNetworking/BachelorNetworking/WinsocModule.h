#ifndef WINSOCMODULE_H
#define WINSOCMODULE_H


#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include "NetworkData.h"
#include "NetworkService.h"

#ifdef _WIN64
#define PACKETOFFSET 8
#else
#define PACKETOFFSET 4
#endif



class WinsocModule
{

private:

	SOCKET	m_ListnerSocket;
	SOCKET	m_ConnectionSocket;

	int		m_ClientID;
	int		m_PacketID;

public:

private:
	bool	AcceptNewClient();
	void	ReadMessagesFromClients();

public:
	WinsocModule();
	~WinsocModule();

	int		Initialize();
	int		Shudown();
	void	Update();
	int		Connect(char* ip);
};

#endif;
