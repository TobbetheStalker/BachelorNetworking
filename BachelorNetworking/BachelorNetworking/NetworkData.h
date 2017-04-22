#ifndef NETWORKDATA_H
#define NETWORKDATA_H

#define MAX_PACKET_SIZE 1000000

#include <string>

enum PacketHeader {
	DEFUALT = 0,
};

struct Packet
{
	PacketHeader	packet_type;
	int				packet_ID;

	virtual void serialize(char * data)
	{			//Turn the PacketType into bytes
		memcpy(data, this, sizeof(Packet));
	}

	virtual void deserialize(char * data)
	{			//Turn bytes into PacketType
		memcpy(this, data, sizeof(Packet));
	}

};

#endif