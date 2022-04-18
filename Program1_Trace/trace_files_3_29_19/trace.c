#include "trace2.h"

int main(int argc, char **argv){
    pcap_t *pcap_open;
    int count = 1;
    
    char errBuf[PCAP_ERRBUF_SIZE];
    
    if ((pcap_open = pcap_open_offline(argv[1], errBuf)) ==  NULL) {
      perror("pcap_open_offline failed\n");
      pcap_close(pcap_open);
      return 0;
   }
    
    pcap_loop(pcap_open, -1, PacketHandler, (u_char *) &count);
    
    pcap_close(pcap_open);
    // printf("Starting main");    
    return 0;
}

void PacketHandler(u_char *arg, const struct pcap_pkthdr *packetHeader, const u_char *packet) {
    uint32_t *packet_number = (uint32_t *) arg;
    printf("\nPacket number: %d  Frame Len: %d\n\n", (*packet_number)++, packetHeader->len);

    switch(EthernetHandler(packet)) {
        case ETHERTYPE_ARP:
            ARPHandler(packet + sizeof(ethHeader));
            break;
        case ETHERTYPE_IP:
            IPHandler(packet + sizeof(ethHeader));
            break;
        default:
            break;
    }
    
}

int EthernetHandler(const u_char *packet) {
    struct ethHeader *ethernet = (struct ethHeader *) packet;
    printf("\tEthernet Header\n");
    printf("\t\tDest MAC: ");
    PrintMacADD(ethernet->destMac);
    printf("\t\tSource MAC: ");
    PrintMacADD(ethernet->srcMac);
    printf("\t\tType: ");

    uint16_t ethType = ntohs(ethernet->etherType);
    // printf("ETHTYPE %d", ethType);

    if (ethType == ETHERTYPE_ARP) {
        printf("ARP\n");
    }
    else if (ethType == ETHERTYPE_IP)
    {
        printf("IP\n");
    }
    else 
    {
        printf("Unknown PDU\n");
    }
    
    
    return ethType;
}

void ARPHandler(const u_char *packet) {
    
    struct arpHeader *arp = (struct arpHeader *) packet;
    uint16_t arpType = ntohs(arp->opcode);
    struct in_addr ip_address;

    
 
    printf("\n");
    printf("\tARP Header\n");
    printf("\t\tOpcode: ");

    if (arpType == ARP_REPLY) {
        printf("Reply\n");
    }
    else if (arpType == ARP_REQUEST) {
        printf("Request\n");
    }
    else {
        printf("Unknown\n");
    }

    printf("\t\tSender MAC: ");
    PrintMacADD(arp->senderMac);

    memcpy(&ip_address.s_addr, &arp->senderIP, sizeof(ip_address.s_addr));
    printf("\t\tSender IP: %s\n", inet_ntoa(ip_address));

    printf("\t\tTarget MAC: ");
    PrintMacADD(arp->targetMac);

    memcpy(&ip_address.s_addr, &arp->targetIP, sizeof(ip_address.s_addr));
    printf("\t\tTarget IP: %s\n", inet_ntoa(ip_address));


};

void IPHandler(const u_char *packet) {
    
    struct ipHeader *ip = (struct ipHeader *) packet;
    struct in_addr ip_address;

    uint8_t ipHdrLength = 4 * (ip->headerLength & 0x0F); 

    printf("\n");
    printf("\tIP Header\n");
    printf("\t\tHeader Len: %u (bytes)\n", ipHdrLength);
    printf("\t\tTOS: 0x0\n");
    printf("\t\tTTL: %u\n", ip->ttl);
    printf("\t\tIP PDU Len: %u (bytes)\n", ip->totalLength / 256);
    printf("\t\tProtocol: ");
    if (ip->protocol == IPTYPE_TCP) {
        printf("TCP\n");
    }
    else if (ip->protocol == IPTYPE_UDP) {
        printf("UDP\n");
    }
    else if (ip->protocol == IPTYPE_ICMP) {
        printf("ICMP\n");
    }
    else {
        printf("Unknown\n");
    }
    


    if (in_cksum((uint16_t *)packet, (uint32_t)ipHdrLength) == 0)
      printf("\t\tChecksum: Correct (0x%04x)\n", ntohs(ip->hdrChecksum));
    else
      printf("\t\tChecksum: Incorrect (0x%04x)\n", ntohs(ip->hdrChecksum));

    memcpy(&ip_address.s_addr, &ip->senderIP, sizeof(ip_address.s_addr));
    printf("\t\tSender IP: %s\n", inet_ntoa(ip_address));

    memcpy(&ip_address.s_addr, &ip->targetIP, sizeof(ip_address.s_addr));
    printf("\t\tDest IP: %s\n", inet_ntoa(ip_address));


    if (ip->protocol == IPTYPE_TCP) {
        TCPHandler(packet + ipHdrLength);
    }
    else if (ip->protocol == IPTYPE_UDP) {
        UDPHandler(packet +ipHdrLength);
    }
    else if (ip->protocol == IPTYPE_ICMP) {
        ICMPHandler(packet + ipHdrLength);
    }
    
};

void UDPHandler(const u_char *packet) {
    struct udpHeader *udp = (struct udpHeader *) packet;
    printf("\n");
    printf("\tUDP Header\n");
    printf("\t\tSource Port : %u\n", ntohs(udp->sourcePort));
    printf("\t\tDest Port : %d\n", ntohs(udp->destPort));
}

void ICMPHandler(const u_char *packet) {
    struct icmpHeader *tcp = (struct icmpHeader *) packet;

    printf("\n");
    printf("\tICMP Header\n");
    if (tcp->type == ICMP_REPLY) {
        printf("\t\tType: Reply\n");
    }
    else if (tcp->type == ICMP_REQUEST) {
        printf("\t\tType: Request\n");
    }
    else {
        printf("\t\tType: %d\n", tcp->type);
    }
}


void TCPHandler(const u_char *packet) {

    struct tcpHeader *tcp = (struct tcpHeader *) packet;
    uint8_t ipHdrLength = 4 * (tcp->headerLen & 0x0F); 

    printf("\n");
    printf("\tTCP Header\n");
    printf("\t\tSource Port: ");
    PrintPort(tcp->sourcePort);
    printf("\t\tDest Port: ");
    PrintPort(tcp->destPort);
    printf("\t\tSequence Number: %u\n",ntohl(tcp->seqNum));
    printf("\t\tACK Number: %u\n", ntohl(tcp->ackNum));
    printf("\t\tACK Flag: %s\n", tcp->flags & TCP_ACK ? "Yes" : "No");
    printf("\t\tSYN Flag: %s\n", tcp->flags & TCP_SYN ? "Yes" : "No");
    printf("\t\tRST Flag: %s\n", tcp->flags & TCP_RST ? "Yes" : "No");
    printf("\t\tFIN Flag: %s\n", tcp->flags & TCP_FIN ? "Yes" : "No");
    printf("\t\tWindow Size: %d\n", ntohs(tcp->windowSize));
    if (in_cksum((uint16_t *)packet, (uint32_t)ipHdrLength) == 0)
      printf("\t\tChecksum: Correct (0x%04x)\n", ntohs(tcp->checksum));
    else
      printf("\t\tChecksum: Incorrect (0x%04x)\n", ntohs(tcp->checksum));
}


void PrintPort(uint16_t portNum) {
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
         printf("POP3");
         break;
      case SMTP:
         printf("SMTP\n");
         break;
      case DNS:
         printf("DNS\n");
         break;
      default:
         printf("%d\n", ntohs(portNum));
         break;
   }
}