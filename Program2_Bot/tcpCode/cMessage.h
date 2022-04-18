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

int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferLen);
int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData);

#endif