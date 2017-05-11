#ifndef NETWORKDATA_H
#define NETWORKDATA_H

#define MAX_PACKET_SIZE 100000
#define BUFFER_SIZE 314572800
//const int GIGABYTE = 1073741824;

#include <string>
#include <chrono>

enum PacketHeader {
	DATA = 0,
	CONNECTION_REQUEST,
	CLOCK_SYNC,
	CLOCK_SYNC_RESPONSE,
	TRANSFER_COMPLETE,
	

};

enum Protocol {
	NONE = 0,
	TCP,
	TCP_WITH_NODELAY,
	UDP,
	RAKNET,
};


struct Packet
{
	PacketHeader	packet_type;	//4 bytes

};

struct DataPacket
{
	PacketHeader	packet_type;		//4 bytes
	int				nrOfPackets;		//4 bytes
	int				ID;					//4 bytes
	char			data[100];		//15000 bytes


};

#pragma pack(1)
struct RakNetPacket
{
	unsigned char	typeId;

};

struct RakNetDataPacket
{
	unsigned char	typeId;
	int				nrOfPackets;		//4 bytes
	int				ID;					//4 bytes
	char			data[100];		//15000 bytes
};


#endif