#ifndef NETWORKDATA_H
#define NETWORKDATA_H

#define MAX_PACKET_SIZE 2000000000
const int GIGABYTE = 1073741824;

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

	//Functions: 4 bytes
	virtual void serialize(char * data)
	{			//Turn the PacketType into bytes
		memcpy(data, this, sizeof(Packet));
	}

	virtual void deserialize(char * data)
	{			//Turn bytes into PacketType
		memcpy(this, data, sizeof(Packet));
	}

};

struct DataPacket
{
	PacketHeader	packet_type;		//4 bytes
	char			data[GIGABYTE];		//1GB

	//Functions: 4 bytes
	virtual void serialize(char * data)
	{			//Turn the PacketType into bytes
		memcpy(data, this, sizeof(Packet));
	}

	virtual void deserialize(char * data)
	{			//Turn bytes into PacketType
		memcpy(this, data, sizeof(Packet));
	}

};

#pragma pack(1)
struct RakNetPacket
{
	unsigned char	typeId;

};


#endif