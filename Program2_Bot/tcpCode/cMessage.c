#include "cMessage.h"
#define MAXBUF 1024


int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData) {

    int sent = 0;
    int temp = lengthOfData;

    uint8_t bufferLength [2];

    uint8_t newDataBuffer[lengthOfData + 2];
    
    int i = 0;
    while (i < lengthOfData) {
        newDataBuffer[i + 2] = dataBuffer[i];
        i++;
    }

    memcpy(bufferLength,(uint8_t*)&(lengthOfData),2);
    newDataBuffer[0] = bufferLength[1];
    newDataBuffer[1] = bufferLength[0];
	sent =  send(socketNumber, newDataBuffer, lengthOfData + 2, 0);
    
    if (sent < 0) {
        return -1;
    }

    return sent - 2;
};

int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferLen) {
	int messageLen = 0;
    int pduLen = 0;

    uint8_t pduHeader[2];
	//now get the data from the client_socket


    pduLen = recv(clientSocket, pduHeader, 2, MSG_WAITALL);
    unsigned int pduPayloadLength = pduHeader[0] | pduHeader[1];
    if (pduLen < 0)
	{
		perror("recv call error");
		return pduLen;
	}
    else if (pduLen == 0) {
        return 0;
    }

    messageLen = recv(clientSocket, dataBuffer, pduPayloadLength, MSG_WAITALL);
	if (messageLen < 0)
    {
       return messageLen;
	}
    else if (messageLen == 0)
    {
        return messageLen;
    }
    else if (messageLen > MAXBUF)
    {
        perror("Message Too Big");
        exit(-1);
    }

    return messageLen;
}

