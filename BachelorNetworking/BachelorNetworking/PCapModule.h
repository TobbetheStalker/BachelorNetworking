#ifndef PCAPMODULE_H
#define PCAPMODULE_H

#include "pcap.h"

class PCapModule
{

private:
	pcap_if_t* m_AllDevices;
	pcap_if_t* m_CurrentDevice;

	int m_nrOfDevices;

public:
	PCapModule();
	~PCapModule();

	int Initialize();
	void Shutdown();

	void PrintDevices();
	void PrintDeviceInformation(pcap_if_t *d);
	char* IpToString(u_long in);
	char* Ip6ToString(struct sockaddr *sockaddr, char *address, int addrlen);

	void SelectDevice(int deviceIndex);
};



#endif
