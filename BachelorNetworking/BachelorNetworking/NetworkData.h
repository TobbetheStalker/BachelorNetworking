#ifndef NETWORKDATA_H
#define NETWORKDATA_H

#define MAX_PACKET_SIZE 1000000

#include <string>
#include <chrono>

enum PacketHeader {
	TEST = 0,
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
	PacketHeader	packet_type;
	int				packet_ID;
	std::chrono::time_point<std::chrono::system_clock> timeStamp;

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