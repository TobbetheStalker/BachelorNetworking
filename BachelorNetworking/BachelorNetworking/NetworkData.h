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
	

};

enum Protocol {
	NONE = 0,
	TCP,
	TCP_WITH_NODELAY,
	UDP,
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

struct SyncPacket : Packet
{
	std::chrono::time_point<std::chrono::system_clock> m_original_time;

	virtual void serialize(char * data)
	{			//Turn the PacketType into bytes
		memcpy(data, this, sizeof(SyncPacket));
	}

	virtual void deserialize(char * data)
	{			//Turn bytes into PacketType
		memcpy(this, data, sizeof(SyncPacket));
	}

};

#endif