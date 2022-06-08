
// 	Writen - HMS April 2017
//  Supports TCP and UDP - both client and server


#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BACKLOG 10

enum SELECT {
   SET_NULL, NOT_NULL
};


typedef struct connection {
    int sk_num;
    struct sockaddr_in6 remote;
    uint32_t len;
} __attribute__((packed)) Connection;

int tcpServerSetup(int serverPort);
int tcpAccept(int mainServerSocket, int debugFlag);
int udpServerSetup(int serverPort);
int tcpClientSetup(char * serverName, char * serverPort, int debugFlag);
int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char * hostName, int serverPort);
int32_t udp_client_setup(char *hostname, uint16_t port_num, Connection *connection);
int32_t select_call(int32_t socket_num, int32_t seconds, int32_t microseconds, int32_t set_null);



#endif
