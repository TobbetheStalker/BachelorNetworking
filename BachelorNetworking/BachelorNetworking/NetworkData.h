#ifndef NETWORKDATA_H
#define NETWORKDATA_H

#define MAX_PACKET_SIZE 100000

#define GIGABYTE = 1073741824;
const int DATA_SIZE = 1073741824;
const int UDP_BUFFER_SIZE = 100000;
const int TCP_BUFFER_SIZE = 1024000 * 300;	//Mb * 300
const int OS_BUFFERS_SIZE = 1024000 * 300;
const int MESSAGE_BUFFER_SIZE = 200;
//const int PING_ITERATIONS = 100;	//Obsoulete
//const int UDP_PACKET_SIZE = 10240;
//const int TCP_PACKET_SIZE = 61440;

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
	char			data[100];		//100 bytes


};

struct RakNetPacket
{
	unsigned char	typeId;

};

struct RakNetDataPacket
{
	unsigned char	typeId;
	char			data[10000];		//100 bytes
};


#endif