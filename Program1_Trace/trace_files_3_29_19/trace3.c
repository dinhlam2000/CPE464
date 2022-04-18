#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <arpa/inet.h>     // ntohs
// #include <netinet/ether.h> // ether_ntoa
#include "checksum.h"

typedef enum {
   ETHERNET, 
   ETHER_ARP, ETHER_IP, 
   IP_ICMP,IP_TCP, IP_UDP,
   UNKNOWN_PDU
} Protocol;

typedef struct ether_addr EtherAddress;

typedef struct {
   uint8_t destHost[6];
   uint8_t srcHost[6];
   uint16_t etherType;
   #define ETHERTYPE_ARP 0x0806
   #define ETHERTYPE_IP 0x0800
} __attribute__((packed)) EtherHeader;

typedef struct {
   uint16_t hdwType;
   uint16_t protType;
   uint8_t hdwAddrLength;
   uint8_t proAddrLength;
   uint16_t opCode;
   uint8_t senderHdwAddr[6];
   uint8_t senderIpAddr[4];
   uint8_t targetHdwAddr[6];
   uint8_t targetIpAddr[4];
   #define ARP_REQUEST 1
   #define ARP_REPLY 2
} __attribute__((packed)) ARPHeader;

typedef struct {
   uint8_t ver_ipHdrLength;
   uint8_t svcType;
   uint16_t totalLength;
   uint16_t id;
   uint16_t flag_fragOffset;
   uint8_t ttl;
   uint8_t prot;
   #define IPTYPE_ICMP 0x01
   #define IPTYPE_TCP 0x06
   #define IPTYPE_UDP 0x11
   uint16_t hdrChecksum;
   uint32_t srcAddr;
   uint32_t destAddr;
} __attribute__((packed)) IPHeader;

typedef struct {
   uint16_t srcPort;
   uint16_t destPort;
   uint32_t seqNum;
   uint32_t ackNum;
   uint8_t offset_rsv;
   uint8_t flags;
   uint16_t winSize;
   uint16_t hdrChecksum;
   uint16_t urgPtr;
} __attribute__((packed)) TCPHeader;

typedef struct {
   uint8_t type;
   uint8_t code;
   uint16_t hdrCheckSum;
   uint16_t id;
   uint16_t seqNum;
   #define ICMP_REQUEST 8
   #define ICMP_REPLY 0
} __attribute__((packed)) ICMPHeader;

typedef struct {
   uint16_t srcPort;
   uint16_t destPort;
   uint16_t hdrLength;
   uint16_t hdrChecksum;
} __attribute__((packed)) UDPHeader;

typedef enum {
   HTTP = 80, Telnet = 23, FTP = 21, POP3 = 110, SMTP = 25, DNS = 53
} Port;

void PacketHandler(u_char *userArg, const struct pcap_pkthdr *pktHeader,
 const u_char *pktdata); 
void EthernetHandler(Protocol *prot, const u_char *pktData);
void ARPHandler(Protocol *prot, const u_char *pktData);
int IPHandler(Protocol *prot, const u_char *pktData);
void TCPHandler(Protocol *prot, const u_char *pktData);
void ICMPHandler(Protocol *prot, const u_char *pktData);
void UDPHandler(Protocol *prot, const u_char *pktData);
void PrintPort(char *str, uint16_t portNum);
void ChecksumTCP(const u_char *tcpData, const u_char *ipData); 

int main(int argc, char **argv) {
   int pktCount = 0;
   char errbuf[PCAP_ERRBUF_SIZE];
   pcap_t *pcap;

   if ((pcap = pcap_open_offline(argv[1], errbuf)) ==  NULL) {
      perror("pcap_open_offline failed\n");
      pcap_close(pcap);
      return EXIT_FAILURE;
   }
   if (pcap_loop(pcap, 0, PacketHandler, (u_char *)&pktCount) < 0) {
      perror("pcap_loop failed\n");
      pcap_close(pcap);
      return EXIT_FAILURE;
   }
   
   pcap_close(pcap);
 
   return 0;
}

void PacketHandler(u_char *userArg, const struct pcap_pkthdr *pktHeader, 
 const u_char *pktData){
   Protocol prot = ETHERNET;
   uint8_t ipHdrLength = 0;
   printf("\nPacket number: %d  Packet Len: %d\n\n",
    ++*(uint32_t *)userArg, pktHeader->len);
    
   while (1) {
      switch(prot) {
         case ETHERNET:
            EthernetHandler(&prot, pktData);
            break;
         case ETHER_ARP:
            ARPHandler(&prot, pktData + sizeof(EtherHeader));
            return;
         case ETHER_IP:
            ipHdrLength = IPHandler(&prot, pktData + sizeof(EtherHeader));
            break;
         case IP_TCP:
            TCPHandler(&prot, pktData + sizeof(EtherHeader) + ipHdrLength);
            ChecksumTCP(pktData + sizeof(EtherHeader) + ipHdrLength,
             pktData + sizeof(EtherHeader));
            return;
         case IP_ICMP:
            ICMPHandler(&prot, pktData + sizeof(EtherHeader) + ipHdrLength);
            return;
         case IP_UDP:
            UDPHandler(&prot, pktData + sizeof(EtherHeader) + ipHdrLength);
            return;
         case UNKNOWN_PDU:
            return;
      } 
   }
}

void EthernetHandler(Protocol *prot, const u_char *pktData) {
   EtherHeader *ethHdr = (EtherHeader *)pktData;
   uint16_t ethType = ntohs(ethHdr->etherType);

   printf("\tEthernet Header\n");
   printf("\t\tDest MAC: %s\n", ether_ntoa((EtherAddress *)ethHdr->destHost));
   printf("\t\tSource MAC: %s\n", ether_ntoa((EtherAddress *)ethHdr->srcHost));
   if (ethType == ETHERTYPE_IP) {
      printf("\t\tType: IP\n\n");
      *prot = ETHER_IP;
   }
   else if (ethType == ETHERTYPE_ARP) {
      printf("\t\tType: ARP\n\n");
      *prot = ETHER_ARP;
   }
   else {
      printf("\t\tType: Unknown PDU\n");
      *prot = UNKNOWN_PDU;
   }
}

void ARPHandler(Protocol *prot, const u_char *pktData) {
   ARPHeader *arpHdr = (ARPHeader *) pktData;
   uint8_t *ipAddr;
 
   printf("\tARP header\n");
   printf("\t\tOpcode: %s\n", 
    ntohs(arpHdr->opCode) == ARP_REPLY ? "Reply" : "Request");

   printf("\t\tSender MAC: %s\n", ether_ntoa((EtherAddress *)arpHdr->senderHdwAddr)); 
   ipAddr = arpHdr->senderIpAddr;
   printf("\t\tSender IP: %d.%d.%d.%d\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);

   printf("\t\tTarget MAC: %s\n", ether_ntoa((EtherAddress *)arpHdr->targetHdwAddr));
   ipAddr = arpHdr->targetIpAddr;
   printf("\t\tTarget IP: %d.%d.%d.%d\n\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
}


int IPHandler(Protocol *prot, const u_char *pktData) {
   IPHeader *ipHdr = (IPHeader *) pktData;
   uint8_t ipHdrLength = 4 * (ipHdr->ver_ipHdrLength & 0x0F); 
   uint8_t *ipAddr;  

   printf("\tIP Header\n");
   printf("\t\tIP Version: %d\n", ipHdr->ver_ipHdrLength >> 4);
   printf("\t\tHeader Len (bytes): %d\n", ipHdrLength);
   printf("\t\tTOS subfields:\n\t\t   Diffserv bits: %d\n\t\t   ECN bits: %d\n",
    ipHdr->svcType >> 2, ipHdr->svcType & ((1 << 2) - 1));
   printf("\t\tTTL: %d\n",ipHdr->ttl);
   switch(ipHdr->prot) {
      case IPTYPE_TCP:         
         printf("\t\tProtocol: TCP\n");
         *prot = IP_TCP;
         break;
      case IPTYPE_ICMP:
         printf("\t\tProtocol: ICMP\n");
         *prot = IP_ICMP;
         break;
      case IPTYPE_UDP:
         printf("\t\tProtocol: UDP\n");
         *prot = IP_UDP;
         break;
      default:
         printf("\t\tProtocol: Unknown\n");
         *prot = UNKNOWN_PDU;
   }
   if (in_cksum((uint16_t *)pktData, (uint32_t)ipHdrLength) == 0)
      printf("\t\tChecksum: Correct ");
   else
      printf("\t\tChecksum: Incorrect ");
   printf("(0x%04x)\n", ntohs(ipHdr->hdrChecksum));
   ipAddr = (uint8_t *)&ipHdr->srcAddr;
   printf("\t\tSender IP: %d.%d.%d.%d\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
   ipAddr = (uint8_t *)&ipHdr->destAddr;
   printf("\t\tDest IP: %d.%d.%d.%d\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);
   if (*prot != UNKNOWN_PDU)
      printf("\n");
   return ipHdrLength;
}


void TCPHandler(Protocol *prot, const u_char *pktData) {
   TCPHeader *tcpHdr = (TCPHeader *) pktData;
   printf("\tTCP Header\n");
   PrintPort("\t\tSource Port: ", tcpHdr->srcPort);
   PrintPort("\t\tDest Port: ", tcpHdr->destPort);
   printf("\t\tSequence Number: %u\n",ntohl(tcpHdr->seqNum));
   printf("\t\tACK Number: %u\n", ntohl(tcpHdr->ackNum));
   printf("\t\tData Offset (bytes): %d\n", 4 * (tcpHdr->offset_rsv >> 4));
   printf("\t\tSYN Flag: %s\n", (tcpHdr->flags >> 1) & 1 ? "Yes" : "No");
   printf("\t\tRST Flag: %s\n", (tcpHdr->flags >> 2) & 1 ? "Yes" : "No");
   printf("\t\tFIN Flag: %s\n", (tcpHdr->flags >> 0) & 1 ? "Yes" : "No");
   printf("\t\tACK Flag: %s\n", (tcpHdr->flags >> 4) & 1 ? "Yes" : "No");
   printf("\t\tWindow Size: %d\n", ntohs(tcpHdr->winSize));
}

void UDPHandler(Protocol *prot, const u_char *pktData) {
   UDPHeader *udpHdr = (UDPHeader *)pktData;
   printf("\tUDP Header\n");
   PrintPort("\t\tSource Port: ", udpHdr->srcPort);
   PrintPort("\t\tDest Port: ", udpHdr->destPort);
}

void ICMPHandler(Protocol *prot, const u_char *pktData) {
   ICMPHeader *icmpHdr = (ICMPHeader *)pktData;
   printf("\tICMP Header\n");
   if (icmpHdr->code) 
      printf("\t\tType: %d\n", icmpHdr->type);
   else if (icmpHdr->type == ICMP_REQUEST)
      printf("\t\tType: Request\n");
   else if (icmpHdr->type == ICMP_REPLY)
      printf("\t\tType: Reply\n");
}

void PrintPort(char *str, uint16_t portNum) {
   printf("%s ", str);
   switch(ntohs(portNum)) {
      case HTTP:
         printf("HTTP\n");
         break;
      case Telnet:
         printf("Telnet\n");
         break;
      case FTP:
         printf("FTP\n");
         break;
      case POP3:
         printf("POP3\n");
         break;
      case SMTP:
         printf("SMTP\n");
         break;
      case DNS:
         printf("DNS\n");
         break;
      default:
         printf("%d\n", ntohs(portNum));
   }
}

#define PSEUDO_HDR_LENGTH 12
void ChecksumTCP(const u_char *tcpData, const u_char *ipData) { 
   IPHeader *ipHdr = (IPHeader *)ipData;
   TCPHeader *tcpHdr = (TCPHeader *)tcpData;
   uint16_t tcpLength = ntohs(ipHdr->totalLength) 
    - 4 * (ipHdr->ver_ipHdrLength & 0x0F);
   uint16_t tcpLengthNet = htons(tcpLength); 
   uint32_t tcpTotalLength = PSEUDO_HDR_LENGTH + tcpLength;
   uint8_t *totalData = calloc(tcpTotalLength, sizeof(uint8_t));
   
   memcpy(totalData, &ipHdr->srcAddr, sizeof(uint32_t));   
   memcpy(totalData + sizeof(uint32_t), &ipHdr->destAddr, sizeof(uint32_t));
   memcpy(totalData + 2 * sizeof(uint32_t) + sizeof(uint8_t), 
    &ipHdr->prot, sizeof(uint8_t));
   memcpy(totalData + 2 * sizeof(uint32_t) + 2*sizeof(uint8_t), 
    &tcpLengthNet, sizeof(uint16_t));
   memcpy(totalData + PSEUDO_HDR_LENGTH, tcpHdr, tcpLength);
   
   if (in_cksum((uint16_t *)totalData, tcpTotalLength))
      printf("\t\tChecksum: Incorrect ");
   else 
      printf("\t\tChecksum: Correct ");
   printf("(0x%04x)\n", ntohs(tcpHdr->hdrChecksum));
   free(totalData);
}
