#ifndef NETWORKSERVICE_H
#define NETWORKSERVICE_H


#include <winsock2.h>
#include <Windows.h>

class NetworkService
{

private:

public:
	static int sendMessage(SOCKET curSocket, char* message, int messageSize);
	static int receiveMessage(SOCKET curSocket, char* buffer, int bufSize);

};



#endif



