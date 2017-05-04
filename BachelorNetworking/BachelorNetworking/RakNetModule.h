#ifndef RAKNETMODULE_H
#define RAKNETMODULE_H

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
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

public:

private:
	void	Clock_Start();
	int		Clock_Stop();
	float	GetAvrgRTT();

public:
	RakNetModule();
	~RakNetModule();

	bool Initialize();
	void Shutdown();

	void Update();

	bool Connect(char* ip);
	void Send(PacketHeader headertype, PacketPriority priority, PacketReliability reliability);
	
	void	Calculate_AVG_Delay();	//TCP
};

#endif