//* Copyright(c) 1999 - 2005 NetGroup, Politecnico di Torino(Italy)
//* Copyright(c) 2005 - 2006 CACE Technologies, Davis(California)
//* All rights reserved.


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
		{
			printf(" (%s)\n", this->m_CurrentDevice->description);
			this->PrintDeviceInformation(this->m_CurrentDevice);
		}
			
		else
			printf(" (No description available)\n");
	}
	
	if (i == 0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
		return;
	}

}

void PCapModule::PrintDeviceInformation(pcap_if_t * d)
{
	pcap_addr_t *a;
	char ip6str[128];

	/* Name */
	printf("%s\n", d->name);

	/* Description */
	if (d->description)
		printf("\tDescription: %s\n", d->description);

	/* Loopback Address*/
	printf("\tLoopback: %s\n", (d->flags & PCAP_IF_LOOPBACK) ? "yes" : "no");

	/* IP addresses */
	for (a = d->addresses; a; a = a->next) {
		printf("\tAddress Family: #%d\n", a->addr->sa_family);

		switch (a->addr->sa_family)
		{
		case AF_INET:
			printf("\tAddress Family Name: AF_INET\n");
			if (a->addr)
				printf("\tAddress: %s\n", this->IpToString(((struct sockaddr_in *)a->addr)->sin_addr.s_addr));
			if (a->netmask)
				printf("\tNetmask: %s\n", this->IpToString(((struct sockaddr_in *)a->netmask)->sin_addr.s_addr));
			if (a->broadaddr)
				printf("\tBroadcast Address: %s\n", this->IpToString(((struct sockaddr_in *)a->broadaddr)->sin_addr.s_addr));
			if (a->dstaddr)
				printf("\tDestination Address: %s\n", this->IpToString(((struct sockaddr_in *)a->dstaddr)->sin_addr.s_addr));
			break;

		case AF_INET6:
			printf("\tAddress Family Name: AF_INET6\n");
			if (a->addr)
				printf("\tAddress: %s\n", this->Ip6ToString(a->addr, ip6str, sizeof(ip6str)));
			break;

		default:
			printf("\tAddress Family Name: Unknown\n");
			break;
		}
	}
	printf("\n");
}

char * PCapModule::IpToString(u_long in)
{

	static char output[12][3 * 4 + 3 + 1];	//12 = Ip buffer
	static short which;
	u_char *p;

	p = (u_char *)&in;
	which = (which + 1 == 12 ? 0 : which + 1);
	_snprintf_s(output[which], sizeof(output[which]), sizeof(output[which]), "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	return output[which];
}

char * PCapModule::Ip6ToString(sockaddr * sockaddr, char * address, int addrlen)
{
	socklen_t sockaddrlen;

#ifdef WIN32
	sockaddrlen = sizeof(struct sockaddr_in6);
#else
	sockaddrlen = sizeof(struct sockaddr_storage);
#endif


	if (getnameinfo(sockaddr,
		sockaddrlen,
		address,
		addrlen,
		NULL,
		0,
		NI_NUMERICHOST) != 0) address = NULL;

	return address;
}
