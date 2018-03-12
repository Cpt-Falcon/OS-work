#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pcap.h>
#include <pcap/pcap.h>
#include <netinet/ether.h>
#include "trace.h"
#include "checksum.h"
#include <netinet/in.h>
#include <arpa/inet.h>

int pcap_next_ex_error_handle(pcap_t *p, struct pcap_pkthdr **pkt_header, 
    const unsigned char **pkt_data)
{
    int errorNum;

    if ((errorNum = pcap_next_ex(p, pkt_header, pkt_data)) == 1)
    {
        if (errorNum == -1)
        {
            perror("Problem with pcap file");
            exit(-1);
        }
        return 1;
    }
    return 0;
}

void getEthernetHeader(ethernet_header *etherHead, 
	const unsigned char *packetData)
{
	struct ether_addr etherTmp;

	printf("	Ethernet Header\n");
    memcpy(etherHead, packetData, sizeof(ethernet_header));
    memcpy(&(etherTmp.ether_addr_octet), etherHead->dest, 
    	ETHERNET_ADDRESS_SIZE);
    printf("		Dest MAC: %s\n", ether_ntoa(&etherTmp));
    
    memcpy(&(etherTmp.ether_addr_octet), etherHead->src, 
    	ETHERNET_ADDRESS_SIZE);
    printf("		Source MAC: %s\n", ether_ntoa(&etherTmp));
}


void getIpHeader(ip_header *ipHead, 
	const unsigned char *packetData)
{
    int ipSize;
    u_short *tempPacketData = (u_short *)packetData;
    struct in_addr ipAddrTmp;

	memcpy(ipHead, packetData, sizeof(ip_header));
    ipSize = (ipHead->version_ihl & 0xF) * WORD_SIZE;

	printf("	IP Header\n");
	printf("		IP Version: %d\n", ipSize);
	printf("		Header Len (bytes): %d\n", ipSize);
	printf("		TOS subfields:\n");
	printf("		   Diffserv bits: %d\n", ipHead->dscp_ecn >> 2);
	printf("		   ECN bits: %d\n", ipHead->dscp_ecn & 0x3);
	printf("		Protocol: %d\n", ipHead->ttl);
    *(((u_short *)tempPacketData) + 5) = 0;

    if (in_cksum((u_short *)packetData, ipSize))
    {
        printf("        Checksum: (Correct) 0x%04x\n", ntohs(in_cksum(tempPacketData, ipSize)));
    }
    else
    {
        printf("        Checksum: (Incorrect) 0x%04x\n", ntohs(in_cksum(tempPacketData, ipSize)));
    }
    memcpy(&(ipAddrTmp.s_addr), ipHead->src, sizeof(uint32_t));
    printf("        Sender IP: %s\n", inet_ntoa(ipAddrTmp));
    memcpy(&(ipAddrTmp.s_addr), ipHead->dest, ETHERNET_ADDRESS_SIZE);
    printf("        Dest IP: %s\n\n", inet_ntoa(ipAddrTmp));
}

void getIpOrArp(ethernet_header *etherHead, ip_header *ipHead, 
	const unsigned char *packetData)
{
	if (etherHead->type == IP_TYPE)
	{
		printf("		Type: IP\n\n");
		getIpHeader(ipHead, packetData + sizeof(ethernet_header));
	}

}

int main(int argc, char *argv[])
{
    char *tracefile = argv[1], errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *pcapFile;
    int packetCount = 1;
    struct pcap_pkthdr *genericHeader;
    const unsigned char *packetData;
    ethernet_header etherHead;
    ip_header ipHead;

    pcapFile = pcap_open_offline(tracefile, errbuf);
    while(pcap_next_ex_error_handle(pcapFile, &genericHeader, &packetData))
    {
    	printf("Packet number: %d  Packet Len: %d\n\n", packetCount, genericHeader->len);
    	getEthernetHeader(&etherHead, packetData);
    	getIpOrArp(&etherHead, &ipHead, packetData);
    	packetCount++;
    }
    printf("tracefile: %s\n", tracefile);
    return 0;
}