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
#include <iostream>
#include <fstream>

#ifdef _WIN64
#define PACKETOFFSET 8
#else
#define PACKETOFFSET 4
#endif

#define RECIVER_PORT "6881"
#define SENDER_PORT "6882"

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
	std::vector<double>	m_packet_loss;
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
	double				highestLoss;
	double				lowestLoss;
	int					currentIteration;

	timeval timeout;
	fd_set fds;

public:

private:
	bool	AcceptNewClient();
	void	ReadMessagesFromClients();

	int		GetMyIp();

	int		TCP_Initialize(bool noDelay, bool isSender);
	int		UDP_Initialize();


	float	GetAvrgRTT();

public:
	WinsocModule();
	~WinsocModule();

	int		Initialize(Protocol newProtocol, bool isSender);
	int		Shutdown();
	void	Update();
	void	TCP_Update();
	void	TCP_WaitForData();
	void	UDP_WaitForData(int packetSize);
	void	UDP_Update();
	int		TCP_Connect(char* ip);

	void	TCP_Send(PacketHeader headertype);
	int		TCP_Send_Data(int packetSize);
	void	UDP_Send(PacketHeader headertype, char* ip);
	int		UDP_Send_Data(char* ip, int iteration, int packetSize);

	int		Calculate_AVG_Delay(int packetsize, int pingIterations);	//TCP
	int		Calculate_AVG_Delay(char* ip, int packetsize, int pingIterations);	//UDP
	void	Clear_PacketLoss_Vector();
	void	Calcualet_Loss();
	bool	GetIsConnected();
	bool	GetTransferComplete();
	void	Clock_Start();
	int		Clock_Stop(bool ms=false);
	int		GetHighest();
	int		GetLowest();
	int		GetLost();
	double	GetAverageLoss();
	double		GetHighestLoss();
	double		GetLowestLoss();
};

#endif;
