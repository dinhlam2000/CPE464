#ifndef __CMESSAGE_H__
#define __CMESSAGE_H__
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "networks.h"

#define INITIAL_PACKET 1
#define POSITIVE_INITIAL_PACKET 2
#define NEGATIVE_INITIAL_PACKET 3
#define BROADCAST_PACKET 4
#define M_MESSAGE 5
#define DEFAULT_MESSAGE_FROM_SERVER 6
#define M_MESSAGE_ERROR_DESTINATION 7
#define CLIENT_EXITING 8
#define ACK_EXITING 9
#define LIST_HANDLES 10
#define LIST_HANDLES_AMOUNT 11
#define HANDLE_RETURN 12
#define ALL_HANDLES_RETURN 13

#define PACKET_FLAG_INDEX 0


int recvPDU(int clientSocket, char * dataBuffer, int bufferLen);
int sendPDU(int socketNumber, char * dataBuffer, int lengthOfData);

#endif