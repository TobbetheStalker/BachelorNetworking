#ifndef RAKNETMODULE_H
#define RAKNETMODULE_H

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetTypes.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "NetworkData.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#define DEFAULT_PORT "6881"

class RakNetModule {

private:
	RakNet::RakPeerInterface* peer;
	RakNet::SocketDescriptor socketDescriptor;

	std::chrono::time_point<std::chrono::steady_clock> m_start_time;
	std::vector<int>	m_ping_times;
	bool				m_ping_in_progress;
	int					m_Avg_Delay;
	bool				transferComplete;
	bool				isConnected;
	int					dataCounter;
	int					data_total;
	int					highest;
	int					lowest;

public:

private:

	float	GetAvrgRTT();

public:
	RakNetModule();
	~RakNetModule();

	bool Initialize();
	void Shutdown();

	void Update();
	void WaitForData();

	bool Connect(char* ip);
	void Send(DefaultMessageIDTypes id, PacketPriority priority, PacketReliability reliability);
	void SendData();
	
	int	Calculate_AVG_Delay(int packetsize);
	void	Clock_Start();
	int		Clock_Stop(bool ms = false);
	bool	GetTransferComplete();
	bool	GetIsConnected();
	int		GetHighest();
	int		GetLowest();
};

#endif