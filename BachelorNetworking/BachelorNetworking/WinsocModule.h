#ifndef WINSOCMODULE_H
#define WINSOCMODULE_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS


#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <sstream>
#include <ws2tcpip.h>
#include <vector>
#include "NetworkData.h"
#include "NetworkService.h"
#include <math.h>

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
	SOCKET	m_TCP_SenderSocket;
	SOCKET	m_TCP_ConenctedSocket;	//TEMP
	SOCKET	m_UDP_Socket;

	int			m_ClientID;
	int			m_PacketID;
	std::string	m_IP;

	Protocol m_CurrentProtocol;
	char* network_data;
	char* UDP_network_data;
	char* network_message;
	sockaddr_in m_RecvAddr;

	std::chrono::time_point<std::chrono::steady_clock> m_start_time;
	std::vector<int>	m_ping_times;
	std::vector<int>	m_packet_loss;
	bool				m_ping_in_progress;
	int					m_Avg_Delay;
	bool				isConnected;
	bool				tranferComplete;
	int					dataCounter;
	int					m_currentID;
	int					m_missedPackets;
	int					data_total;
	int					highest;
	int					lowest;
	double				averageLoss;
	int					highestLoss;
	int					lowestLoss;

	timeval timeout;
	fd_set fds;

public:

private:
	bool	AcceptNewClient();
	void	ReadMessagesFromClients();

	int		GetMyIp();

	int		TCP_Initialize(bool noDelay);
	int		UDP_Initialize();


	float	GetAvrgRTT();

public:
	WinsocModule();
	~WinsocModule();

	int		Initialize(Protocol newProtocol);
	int		Shutdown();
	void	Update();
	void	TCP_Update();
	void	TCP_WaitForData();
	void	UDP_WaitForData();
	void	UDP_Update();
	int		TCP_Connect(char* ip);

	void	TCP_Send(PacketHeader headertype);
	int		TCP_Send_Data();
	void	UDP_Send(PacketHeader headertype, char* ip);
	int		UDP_Send_Data(char* ip);

	int		Calculate_AVG_Delay();	//TCP
	int		Calculate_AVG_Delay(char* ip);	//UDP
	void	Clear_PacketLoss_Vector();
	void	Calcualet_Loss();
	bool	GetIsConnected();
	bool	GetTransferComplete();
	void	Clock_Start();
	int		Clock_Stop(bool ms=false);
	int		GetHighest();
	int		GetLowest();
	double	GetAverageLoss();
	int		GetHighestLoss();
	int		GetLowestLoss();
};

#endif;
