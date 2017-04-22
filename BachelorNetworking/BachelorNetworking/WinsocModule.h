#ifndef WINSOCMODULE_H
#define WINSOCMODULE_H


#pragma comment(lib, "Ws2_32.lib")

#include "NetworkData.h"
#include "NetworkService.h"

class WinsocModule
{

private:

	SOCKET	m_ListnerSocket;
	SOCKET	m_ConnectionSocket;

	int		m_PacketID;

public:
	WinsocModule();
	~WinsocModule();

	int		Initialize();
	int		Shudown();
	void	Update();
	int		Connect(char* ip);
};

#endif;
