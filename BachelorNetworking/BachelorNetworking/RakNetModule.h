#ifndef RAKNETMODULE_H
#define RAKNETMODULE_H

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "NetworkData.h"
#include <vector>

#define DEFAULT_PORT "6881"

class RakNetModule {

private:
	RakNet::RakPeerInterface* peer;
	RakNet::SocketDescriptor listner;

	std::chrono::time_point<std::chrono::steady_clock> m_start_time;
	std::vector<int>	m_ping_times;
	bool				m_ping_in_progress;
	int					m_Avg_Delay;
	bool				transferComplete;

public:

private:

	float	GetAvrgRTT();

public:
	RakNetModule();
	~RakNetModule();

	bool Initialize();
	void Shutdown();

	void Update();

	bool Connect(char* ip);
	void Send(DefaultMessageIDTypes id, PacketPriority priority, PacketReliability reliability);
	
	int	Calculate_AVG_Delay();
	void	Clock_Start();
	int		Clock_Stop();
	bool	GetTransferComplete();
};

#endif