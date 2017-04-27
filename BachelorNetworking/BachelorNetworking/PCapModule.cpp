#include "PCapModule.h"

PCapModule::PCapModule()
{
	this->m_AllDevices = nullptr;
	this->m_CurrentDevice = nullptr;
}

PCapModule::~PCapModule()
{
}

int PCapModule::Initialize()
{
	char errbuf[PCAP_ERRBUF_SIZE];

	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL /* auth is not needed */, &m_AllDevices, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs_ex: %s\n", errbuf);
		
		return 0;
	}

	this->PrintDevices();

	return 1;
}

void PCapModule::Shutdown()
{
	if (this->m_AllDevices != nullptr)
	{
		pcap_freealldevs(this->m_AllDevices);
	}
}

void PCapModule::PrintDevices()
{
	/* Print the list */
	int i = 0;
	for (this->m_CurrentDevice = this->m_AllDevices; this->m_CurrentDevice != NULL; this->m_CurrentDevice = this->m_CurrentDevice->next)
	{
		printf("%d. %s", ++i, this->m_CurrentDevice->name);
		if (this->m_CurrentDevice->description)
			printf(" (%s)\n", this->m_CurrentDevice->description);
		else
			printf(" (No description available)\n");
	}
	
	if (i == 0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
		return;
	}

}
