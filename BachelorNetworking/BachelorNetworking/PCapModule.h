#ifndef PCAPMODULE_H
#define PCAPMODULE_H

#include "pcap.h"

class PCapModule
{

private:
	pcap_if_t* m_AllDevices;
	pcap_if_t* m_CurrentDevice;

public:
	PCapModule();
	~PCapModule();

	int Initialize();
	void Shutdown();

	void PrintDevices();
};



#endif
