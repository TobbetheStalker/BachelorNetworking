#ifndef PCAPMODULE_H
#define PCAPMODULE_H

#include <string>
#include "pcap.h"

#define DEFAULT_PORT "6881"

//Function created problem while a member of the class
void Packet_Callback(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);

class PCapModule
{

private:
	pcap_if_t* m_AllDevices;
	pcap_if_t* m_CurrentDevice;

	pcap_t* m_adHandle;

	int m_nrOfDevices;

private:
	int SetFilter(std::string filter);

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
	int StartCapture(std::string filter);

};



#endif
