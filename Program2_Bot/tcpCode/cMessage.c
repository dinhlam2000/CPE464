#include "cMessage.h"
#define MAXBUF 1024


int sendPDU(int socketNumber, char * dataBuffer, int lengthOfData) {

    int sent = 0;
    uint16_t temp;


    uint8_t newDataBuffer[lengthOfData + 2];
    
    memcpy(newDataBuffer + 2, dataBuffer , lengthOfData);
    // while (i < lengthOfData) {
    //     newDataBuffer[i + 2] = dataBuffer[i];
    //     i++;
    // }
    temp = htons(lengthOfData + 2);
    memcpy(newDataBuffer,(uint8_t*)&(temp),2);
    // newDataBuffer[0] = bufferLength[1];
    // newDataBuffer[1] = bufferLength[0];
	sent =  send(socketNumber, newDataBuffer, lengthOfData + 2, 0);
    
    if (sent < 0) {
        return -1;
    }

    return sent - 2;
};

int recvPDU(int clientSocket, char * dataBuffer, int bufferLen) {
	int messageLen = 0;
    int pduLen = 0;

    uint8_t pduHeader[2];
	//now get the data from the client_socket


    pduLen = recv(clientSocket, pduHeader, 2, MSG_WAITALL);
    uint16_t pduPayloadLength =  0;
    memcpy(&pduPayloadLength, pduHeader, 2);
    pduPayloadLength = ntohs(pduPayloadLength) - 2;
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

