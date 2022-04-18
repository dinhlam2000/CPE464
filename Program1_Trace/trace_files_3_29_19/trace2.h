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

#define ETHERNET_ADDRESS_LENGTH 6
#define ETHERNET_STRUCT_LENGTH 14
#define IP_ADDRESS_LENGTH 4
#define ARP_BASE_LEN 8
#define IP_HEADER_SIZE 20

// TCP FLAGS
#define TCP_ACK 0x10
#define TCP_PSH 0x08
#define TCP_RST 0x04
#define TCP_SYN 0x02
#define TCP_FIN 0x01

// ETHERNET TYPE
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IP 0x0800

// ARP TYPE
#define ARP_REQUEST 1
#define ARP_REPLY 2

// IP TYPE
#define IPTYPE_ICMP 1
#define IPTYPE_TCP 6
#define IPTYPE_UDP 17

// IMCP TYPE
#define ICMP_REPLY 0                                //reply (type = 0 code = 0)
#define ICMP_REQUEST 8                              //request (type = 8 code = 0)

// PORTS
typedef enum {
   HTTP = 80, Telnet = 23, FTP = 21, POP3 = 110, SMTP = 25, DNS = 53
} Port;

// HELPER FUNC
#define PrintMacADD(packet) (printf("%s\n", ether_ntoa((struct ether_addr *) packet)))



typedef struct ethHeader {
    uint8_t destMac[ETHERNET_ADDRESS_LENGTH];       //dest mac address
    uint8_t srcMac[ETHERNET_ADDRESS_LENGTH];       //src mac address
    uint16_t etherType;                         //ethernet type
} __attribute__((packed)) ethHeader;


typedef struct arpHeader {
    uint16_t    hwType;                             //hardware type
    uint16_t    pType;                              //protocol type
    uint8_t     hwAddrLength;                       //hardware address length
    uint8_t     pAddrLength;                        //protocol address length
    uint16_t    opcode;                             //opcode
    uint8_t     senderMac[ETHERNET_ADDRESS_LENGTH];       //sender mac address
    uint8_t     senderIP[IP_ADDRESS_LENGTH];              //sender IP address
    uint8_t     targetMac[ETHERNET_ADDRESS_LENGTH];       //target mac address
    uint8_t     targetIP[IP_ADDRESS_LENGTH];              //target IP address
} __attribute__((packed)) arpHeader;


typedef struct ipHeader{
   uint8_t headerLength;
   uint8_t id1;
   uint16_t totalLength;
   uint16_t id2;
   uint16_t offset;
   uint8_t ttl;
   uint8_t protocol;
   uint16_t hdrChecksum;
   uint8_t  senderIP[IP_ADDRESS_LENGTH];              //sender IP address
   uint8_t targetIP[IP_ADDRESS_LENGTH];              //target IP address
} __attribute__((packed)) ipHeader;

typedef struct tcpHeader {
    uint16_t sourcePort;
    uint16_t destPort;
    uint32_t seqNum;
    uint32_t ackNum;
    uint8_t headerLen;
    uint8_t flags;
    uint16_t windowSize;
    uint16_t checksum;
    uint16_t id;
} __attribute__((packed)) tcpHeader;

typedef struct udpHeader {
    uint16_t sourcePort;
    uint16_t destPort;
} __attribute__((packed)) udpHeader;

typedef struct icmpHeader {
    uint8_t type;
} __attribute__((packed)) icmpHeader;

typedef enum {
   ETHERNET, 
   ETHER_ARP, ETHER_IP, 
   IP_ICMP,IP_TCP, IP_UDP,
   UNKNOWN_PDU
} Protocol;


void PacketHandler(u_char *arg, const struct pcap_pkthdr *packetHeader, const u_char *packet);
void ARPHandler(const u_char *packet);
void IPHandler(const u_char *packet);
void UDPHandler(const u_char *packet);
void TCPHandler(const u_char *packet);
void ICMPHandler(const u_char *packet);
void PrintPort(uint16_t portNum);
int EthernetHandler(const u_char *packet);