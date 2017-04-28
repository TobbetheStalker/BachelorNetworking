//* Copyright(c) 1999 - 2005 NetGroup, Politecnico di Torino(Italy)
//* Copyright(c) 2005 - 2006 CACE Technologies, Davis(California)
//* All rights reserved.
//Code Modified by Tobias Kling

#include "PCapModule.h"

PCapModule::PCapModule()
{
	this->m_AllDevices = nullptr;
	this->m_CurrentDevice = nullptr;
	this->m_adHandle = nullptr;

	this->m_nrOfDevices = 0;
}

PCapModule::~PCapModule()
{
	this->Shutdown();
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
		delete this->m_AllDevices;
		this->m_AllDevices = nullptr;
	}
	if (this->m_CurrentDevice != nullptr)
	{
		pcap_freealldevs(this->m_CurrentDevice);
		delete this->m_CurrentDevice;
		this->m_CurrentDevice = nullptr;
	}
	if (this->m_adHandle != nullptr)
	{
		delete this->m_adHandle;
		this->m_adHandle = nullptr;
	}

	this->m_nrOfDevices = 0;
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
	}

	this->m_nrOfDevices = i;

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

void PCapModule::SelectDevice(int deviceIndex)
{
	if (deviceIndex <= this->m_nrOfDevices && deviceIndex > 0)
	{
		this->m_CurrentDevice = this->m_AllDevices;
		for (int i = 1; i < deviceIndex; i++)
		{
			this->m_CurrentDevice = this->m_CurrentDevice->next;		
		}
		printf("Selected Device: %d \n", deviceIndex);
	}
	else
	{
		printf("Invalid Index \n");
	}
}

int PCapModule::StartCapture(std::string filter)
{
	char errbuf[PCAP_ERRBUF_SIZE];

	/* Open the device */
	if ((this->m_adHandle = pcap_open(this->m_CurrentDevice->name,          // name of the device
		65536,            // portion of the packet to capture
						  // 65536 guarantees that the whole packet will be captured on all the link layers
		PCAP_OPENFLAG_PROMISCUOUS,    // promiscuous mode
		1000,             // read timeout
		NULL,             // authentication on the remote machine
		errbuf            // error buffer
	)) == NULL)
	{
		fprintf(stderr, "\nUnable to open the adapter. %s is not supported by WinPcap\n", this->m_CurrentDevice->name);

		return 0;
	}

	this->SetFilter(filter);

	printf("\nlistening on %s...\n", this->m_CurrentDevice->description);

	/* start the capture */
	pcap_loop(this->m_adHandle, 0, Packet_Callback, NULL);

	return 1;
}

int PCapModule::SetFilter(std::string filter)
{
	struct bpf_program fcode;
	u_int netmask;

	if (pcap_datalink(this->m_adHandle) != DLT_EN10MB)
	{
		fprintf(stderr, "\nThis program works only on Ethernet networks.\n");
		return 0;
	}

	if (this->m_CurrentDevice->addresses != NULL)
	{
		/* Retrieve the mask of the first address of the interface */
		netmask = ((struct sockaddr_in *)(this->m_CurrentDevice->addresses->netmask))->sin_addr.S_un.S_addr;
	}
	else
	{
		/* If the interface is without an address we suppose to be in a C class network */
		netmask = 0xffffff;
	}


	//compile the filter
	if (pcap_compile(this->m_adHandle, &fcode, filter.c_str(), 1, netmask) < 0)
	{
		fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");
		/* Free the device list */
		pcap_freealldevs(this->m_AllDevices);
		return 0;
	}

	//set the filter
	if (pcap_setfilter(this->m_adHandle, &fcode) < 0)
	{
		fprintf(stderr, "\nError setting the filter.\n");
		/* Free the device list */
		pcap_freealldevs(this->m_AllDevices);
		return 0;
	}

	return 1;
}

void Packet_Callback(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	//What to do when a packet comes through the device

	struct tm ltime;
	char timestr[16];
	time_t local_tv_sec;

	(VOID)(param);
	(VOID)(pkt_data);

	/* convert the timestamp to readable format */
	local_tv_sec = header->ts.tv_sec;
	localtime_s(&ltime, &local_tv_sec);
	strftime(timestr, sizeof timestr, "%H:%M:%S", &ltime);

	printf("%s,%.6d len:%d\n", timestr, header->ts.tv_usec, header->len);
}
