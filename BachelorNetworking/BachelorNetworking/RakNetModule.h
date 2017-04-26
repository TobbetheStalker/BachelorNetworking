#ifndef RAKNETMODULE_H
#define RAKNETMODULE_H

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "NetworkData.h"

#define DEFAULT_PORT "6881"

class RakNetModule {

private:
	RakNet::RakPeerInterface* peer;
	RakNet::SocketDescriptor listner;

public:
	RakNetModule();
	~RakNetModule();

	bool Initialize();
	void Shutdown();

	void Update();

	bool Connect(char* ip);
	void Send(PacketHeader headertype, PacketPriority priority, PacketReliability reliability);
};

#endif