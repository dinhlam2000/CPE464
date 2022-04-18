//
//  main.c
//  Trace
//
//  Created by Long Tran on 1/14/17.
//  Copyright © 2017 Long Tran. All rights reserved.
//

#include <pcap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <netinet/ether.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include "checksum.h"


#pragma pack(1)

#define ETHERNET_ADDR_LEN 6
#define ETHERNET_STRUCT_LENGTH 14
#define IP_ADDR_LEN 4
#define ARP_BASE_LEN 8

#define	ARP_REQUEST	1	/* request to resolve address */
#define	ARP_REPLY	2	/* response to previous request */

struct sEthernet {
    uint8_t dHost[ETHERNET_ADDR_LEN];   //destination source address
    uint8_t sHost[ETHERNET_ADDR_LEN];   //source host address
    u_short type;                       //Layer 3 type IP ? ARP...
} sEthernet;

struct sARP_header {
    uint16_t    hwType;                             //hardware type
    uint16_t    pType;                              //protocol type
    uint8_t     hwAddrLength;                       //hardware address length
    uint8_t     pAddrLength;                        //protocol address length
    uint16_t    opcode;                             //opcode
    uint8_t     senderMac[ETHERNET_ADDR_LEN];       //sender mac address
    uint8_t     senderIP[IP_ADDR_LEN];              //sender IP address
    uint8_t     targetMac[ETHERNET_ADDR_LEN];       //target mac address
    uint8_t     targetIP[IP_ADDR_LEN];              //target IP address
    
};

struct sIP {
    uint8_t verHeader;                              // 4-7 = version 0-3 = header length
#define IP_VERSION(ip)      (((ip)->verHeader) >> 4)
#define IP_HeaderLen(ip)    ((((ip)->verHeader) & 0x0F) << 2)
    uint8_t servType;                               //service type
    uint16_t totalLen;                              //total length
    uint16_t id;                                    //identification
    uint16_t offset;                                //flag + fragment offset
    uint8_t ttl;                                    //TTL
    uint8_t protocol;                               //protocol
    uint16_t checksum;                              //header checksum
    struct in_addr sIPAddr;                         //source IP Address
    struct in_addr dIPAddr;                         //destination IP Address
    uint32_t option;                                //13-31 = option 0-12 = padding
};

struct sICMP {
    uint8_t type;                                   //ICMP type
#define ICMP_REPLY 0                                //reply (type = 0 code = 0)
#define ICMP_REQUEST 8                              //request (type = 8 code = 0)
    uint8_t code;                                   //code
    uint16_t checksum;                              //checksum
    uint32_t other;                                 //rest of header
};

struct sTCP {
    uint16_t sPort;                                 //Source port
    uint16_t dPort;                                 //Destination port
    uint32_t seqNum;                                //sequence Number
    uint32_t ack;                                   //acknowledgement number
    uint8_t  offx2;                                 //4-7 = data offset, 1-3 = reserve, 0 = NS flag
#define TCP_HeaderLen(tcp) ((((tcp)->offx2) >> 4) << 2)
    uint8_t flag;                                   //flag
#define TCP_CWR 0x80
#define TCP_ECE 0x40
#define TCP_URG 0x20
#define TCP_ACK 0x10
#define TCP_PSH 0x08
#define TCP_RST 0x04
#define TCP_SYN 0x02
#define TCP_FIN 0x01
    uint16_t wdsize;                                //window size
    uint16_t checksum;                              //checksum
    uint16_t urp;                                   //urgent Pointer

};

struct sUDP {
    uint16_t sPort;                                 //source port
    uint16_t dPort;                                 //destination port
    uint16_t len;                                   //header length
    uint16_t checksum;                              //checksum;
};

#define FTP1_PORT 20
#define FTP2_PORT 21
#define TELNET1_PORT 23
#define TELNET2_PORT 24
#define SMTP1_PORT 25
#define SMTP2_PORT 26
#define DNS_PORT 53
#define HTTP_PORT 80
#define POP3_PORT 110

#define PrintMacADD(packet) (printf("%s\n", ether_ntoa((struct ether_addr *) packet)))

void ProcessPakcet(u_char *arg, const struct pcap_pkthdr *packetHeader, const u_char *packet);
//Layer 2
int ProcessEthernet(const u_char *packet);

//Layer 3
void ProcessARP(const u_char *packet);
void ProcessIP(const u_char *packet);

//Layer 4
void ProcessICMP(const u_char *packet);
void ProcessTCP(struct sIP * ip, struct sTCP * tcp);
void ProcessUDP(const u_char *packet);

int TCPChecksum(struct sIP * ip, struct sTCP * tcp);
void PrintTCPUDPPort(uint16_t port);
//char filename[200] = "/Users/longtran/Google Drive/Cal Poly/CPE 461/Program 1/Program1_Trace/trace_files_3_26_16/ArpTest.pcap";

//char filename[200] = "/Users/longtran/Google Drive/Cal Poly/CPE 461/Program 1/Program1_Trace/trace_files_3_26_16/Http.pcap";

//char filename[200] = "/Users/longtran/Google Drive/Cal Poly/CPE 461/Program 1/Program1_Trace/trace_files_3_26_16/IP_bad_checksum.pcap";

//char filename[200] = "/Users/longtran/Google Drive/Cal Poly/CPE 461/Program 1/Program1_Trace/trace_files_3_26_16/largeMix.pcap";

//char filename[200] = "/Users/longtran/Google Drive/Cal Poly/CPE 461/Program 1/Program1_Trace/trace_files_3_26_16/largeMix2.pcap";

//char filename[200] = "/Users/longtran/Google Drive/Cal Poly/CPE 461/Program 1/Program1_Trace/trace_files_3_26_16/PingTest.pcap";

//char filename[200] = "/Users/longtran/Google Drive/Cal Poly/CPE 461/Program 1/Program1_Trace/trace_files_3_26_16/smallTCP.pcap";

//char filename[200] = "/Users/longtran/Google Drive/Cal Poly/CPE 461/Program 1/Program1_Trace/trace_files_3_26_16/TCP_bad_checksum.pcap";

//char filename[200] = "/Users/longtran/Google Drive/Cal Poly/CPE 461/Program 1/Program1_Trace/trace_files_3_26_16/UDPfile.pcap";

struct sTCP_pseudo /*the tcp pseudo header*/
{
    uint32_t sIPAddr;
    uint32_t dIPAddr;
    uint8_t zero;
    uint8_t protocol;
    uint16_t length;
};


int main(int argc, const char * argv[]) {
    // insert code here...
    pcap_t *handle_pcap;
    int count = 1;
    
    char errBuf[PCAP_ERRBUF_SIZE];
    
    if (!(handle_pcap = pcap_open_offline(argv[1], errBuf))) {
        printf("Not able to open pcap file\n");
        return 0;
    }
    
    pcap_loop(handle_pcap, -1, ProcessPakcet, (u_char *) &count);
    
    pcap_close(handle_pcap);
    
    return 0;
}

void ProcessPakcet(u_char *arg, const struct pcap_pkthdr *packetHeader, const u_char *packet) {
    int *count = (int *) arg;
    printf("\nPacket number: %d  Packet Len: %d\n\n", (*count)++, packetHeader->len);
    
    switch (ProcessEthernet(packet)) {
        case ETHERTYPE_ARP :
            printf("ARP\n");
            ProcessARP(packet + ETHERNET_STRUCT_LENGTH);
            break;
        case ETHERTYPE_IP :
            printf("IP\n");
            ProcessIP(packet + ETHERNET_STRUCT_LENGTH);
            break;
        default :
            printf("Unknown PDU\n");
            break;
            
    }
    
}

int ProcessEthernet(const u_char *packet) {
    struct sEthernet *eth = (struct sEthernet *) packet;
    printf("\tEthernet Header\n");
    printf("\t\tDest MAC: ");
    PrintMacADD(eth->dHost);
    printf("\t\tSource MAC: ");
    PrintMacADD(eth->sHost);
    printf("\t\tType: ");
    
    return ntohs(eth->type);
}

void ProcessARP(const u_char *packet) {
    struct sARP_header *arp = (struct sARP_header *) packet;
    struct in_addr temp;
    
    printf("\n\tARP header\n");
    
    printf("\t\tOpcode: ");
    
    switch(ntohs(arp->opcode)) {
        case ARP_REQUEST:
            printf("Request\n");
            break;
        case ARP_REPLY :
            printf("Reply\n");
            break;
//        case ARPOP_REVREQUEST :
//            printf("RevRequest\n");
//            break;
//        case ARPOP_REVREPLY :
//            printf("RevReply\n");
//            break;
//        case ARPOP_INVREQUEST :
//            printf("InvRequest\n");
//            break;
//        case ARPOP_INVREPLY :
//            printf("InvReply\n");
//            break;
        default :
            printf("\n");
            break;
    }
    
    printf("\t\tSender MAC: ");
    PrintMacADD(arp->senderMac);
    
    memcpy(&temp.s_addr, &arp->senderIP, sizeof(temp.s_addr));
    printf("\t\tSender IP: %s\n", inet_ntoa(temp));
    
    printf("\t\tTarget MAC: ");
    PrintMacADD(arp->targetMac);
    
    memcpy(&temp.s_addr, &arp->targetIP, sizeof(temp.s_addr));
    printf("\t\tTarget IP: %s\n", inet_ntoa(temp));
    
    printf("\n");
}

void ProcessIP(const u_char *packet) {
    struct sIP *ip = (struct sIP *) packet;
    u_short temp;
    
    printf("\n\tIP Header\n");
    printf("\t\tIP Version: %d\n", IP_VERSION(ip));
    printf("\t\tHeader Len (bytes): %d\n", IP_HeaderLen(ip));
    printf("\t\tTOS subfields:\n");
    printf("\t\t   Diffserv bits: %d\n", (ip->servType & 0xFC) >> 2);
    printf("\t\t   ECN bits: %d\n", ip->servType & 0x03);
    printf("\t\tTTL: %d\n", ip->ttl);
    printf("\t\tProtocol: ");
    
    switch(ip->protocol) {
        case IPPROTO_ICMP :
            printf("ICMP\n");
            break;
        case IPPROTO_TCP :
            printf("TCP\n");
            break;
        case IPPROTO_UDP :
            printf("UDP\n");
            break;
        default:
            printf("Unknown\n");
            break;
    }
    temp = in_cksum((u_short *) packet, IP_HeaderLen(ip));
    printf("\t\tChecksum: %s (0x%04x)\n", temp == 0 ? "Correct" : "Incorrect", ntohs(ip->checksum));
    
    printf("\t\tSender IP: %s\n", inet_ntoa(ip->sIPAddr));
    printf("\t\tDest IP: %s\n", inet_ntoa(ip->dIPAddr));
    switch(ip->protocol) {
        case IPPROTO_ICMP :
            ProcessICMP(packet + IP_HeaderLen(ip));
            break;
        case IPPROTO_TCP :
            ProcessTCP((struct sIP *) packet, (struct sTCP *) (packet + IP_HeaderLen(ip)));
            break;
        case IPPROTO_UDP :
            ProcessUDP(packet + IP_HeaderLen(ip));
            break;
        default:
            //printf("Unknown PDU\n");
            break;
    }
    
    
    
}

void ProcessICMP(const u_char *packet) {
    struct sICMP *icmp = (struct sICMP *) packet;
    printf("\n\tICMP Header\n");
    printf("\t\tType: ");
    
    if (icmp->type == ICMP_REPLY && icmp->code == 0) {
        printf("Reply\n");
    }
    else if (icmp->type == ICMP_REQUEST && icmp->code == 0) {
        printf("Request\n");
    }
    else {
        printf("%d\n", icmp->type);
    }
}

void ProcessTCP(struct sIP * ip, struct sTCP * tcp) {
    printf("\n\tTCP Header\n");
    
    printf("\t\tSource Port:  ");
    PrintTCPUDPPort(ntohs(tcp->sPort));
    
    printf("\t\tDest Port:  ");
    PrintTCPUDPPort(ntohs(tcp->dPort));
    
    printf("\t\tSequence Number: %u\n", ntohl(tcp->seqNum));
    printf("\t\tACK Number: %u\n", ntohl(tcp->ack));
    
    printf("\t\tData Offset (bytes): %u\n", TCP_HeaderLen(tcp));
    
    printf("\t\tSYN Flag: %s\n", tcp->flag & TCP_SYN ? "Yes" : "No");
    printf("\t\tRST Flag: %s\n", tcp->flag & TCP_RST ? "Yes" : "No");
    printf("\t\tFIN Flag: %s\n", tcp->flag & TCP_FIN ? "Yes" : "No");
    printf("\t\tACK Flag: %s\n", tcp->flag & TCP_ACK ? "Yes" : "No");
    printf("\t\tWindow Size: %u\n", ntohs(tcp->wdsize));
    printf("\t\tChecksum: %s (0x%04x)\n", TCPChecksum(ip,tcp) == 0 ? "Correct" : "Incorrect", ntohs(tcp->checksum));
}


void ProcessUDP(const u_char *packet) {
    struct sUDP *udp = (struct sUDP *) packet;
    
    printf("\n\tUDP Header\n");
    printf("\t\tSource Port:  ");
    
    PrintTCPUDPPort(ntohs(udp->sPort));
    
    printf("\t\tDest Port:  ");
    PrintTCPUDPPort(ntohs(udp->dPort));
}

int TCPChecksum(struct sIP *ip, struct sTCP *tcp) {
    int totalTCPLen, checksum;
    uint16_t dataLen = ntohs(ip->totalLen) - IP_HeaderLen(ip) - TCP_HeaderLen(tcp);
    struct sTCP_pseudo pseudo;
    pseudo.dIPAddr = ip->dIPAddr.s_addr;
    pseudo.sIPAddr = ip->sIPAddr.s_addr;
    pseudo.protocol = ip->protocol;
    pseudo.zero = 0;
    pseudo.length = htons(TCP_HeaderLen(tcp) + dataLen);
    
    totalTCPLen = dataLen + TCP_HeaderLen(tcp) + sizeof(pseudo);        //total length = pseudo length + TCP header length + data length
    uint8_t *data = malloc(sizeof(uint8_t) * totalTCPLen);
    memcpy(data, &pseudo, sizeof(pseudo));                              //copy pseudo header
    memcpy(data + sizeof(pseudo), tcp, TCP_HeaderLen(tcp) + dataLen);   //copy TCP header + TCP data
    checksum = in_cksum((uint16_t *) data,totalTCPLen);
    free(data);
    return checksum;
}

void PrintTCPUDPPort(uint16_t port) {
    switch (port) {
        case FTP1_PORT:
        case FTP2_PORT:
            printf("FTP\n");
            break;
            
        case TELNET1_PORT:
        case TELNET2_PORT:
            printf("Telnet\n");
            break;
            
        case SMTP1_PORT:
        case SMTP2_PORT:
            printf("SMTP\n");
            break;
            
        case DNS_PORT:
            printf("DNS\n");
            break;

        case HTTP_PORT:
            printf("HTTP\n");
            break;
            
        case POP3_PORT:
            printf("POP3\n");
            break;
            
        default :
            printf("%d\n", port);
    }

}

