/******************************************************************************
* myClient.c
*
*****************************************************************************/

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
#include "cMessage.h"
#include "pollLib.h"
#include "sharedStuffs.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1


int sendToServer(int socketNum, int senderLength, char *senderHandle);
int readFromStdin(char * buffer);
void checkArgs(int argc, char * argv[]);
void setupClientPoll(int socketNum);
int processServer(int socketNumber);
int sendInitialPacket(char * handleName, int serverSocket);
int sendInputAsPacket(int socketNum, char * buffer, int senderLength, char *senderHandle);

#define PrintDollarSign() do {\
                            printf("$: ");\
                            fflush(stdout);\
                            } while(0)

int main(int argc, char * argv[])
{
    checkArgs(argc, argv);

	int socketNum = 0;         //socket descriptor
	char * pduSend;
	int socketReady = 0;
	int pduReturned;

	int senderLength = strlen(argv[1]);

    char *clientHandleName = malloc(sizeof(char) * (senderLength + 1)); //add one to take account for that '\0'
    memcpy(clientHandleName,argv[1],senderLength);
    clientHandleName[senderLength] = '\0';
    // printf("ClientHandleName %s\n", clientHandleName);
    


	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	// printf("SocketNum %d\n", socketNum);
	setupClientPoll(socketNum);
    senderLength = sendInitialPacket(argv[1], socketNum);
	// pduSend = sendToServer(socketNum);

	while (1) {
		// printf("Blocking\n");
		socketReady = pollCall(-1);
		// printf("Socketready %d\n", socketReady);
		if (socketReady == socketNum)
		{
			// Receiv pdu and print it out
			// printf("Socket Readyy for server= %d \n", socketReady);
			pduReturned = processServer(socketReady);
            if (pduReturned == NEGATIVE_INITIAL_PACKET) {
                break;
            }
		}
		else {
			// printf("Socket Readyy = %d \n", socketReady);

			pduSend = sendToServer(socketNum, senderLength, clientHandleName);
			if (pduSend == -1) {
				// close(socketReady);
				break;
			}
		}
	}
	// while (strcmp(pduSend, exit) != 0) {
	// 	pduSend = sendToServer(socketNum);
	// }
	
	close(socketNum);
	
	return 0;
}

int sendInitialPacket(char * handleName, int serverSocket) {

    uint8_t bufferLength [2];
    int handleLen= strlen(handleName);
    int pduLength = handleLen + 5; //first extra 4 byte is for the packet set up and last byte is \0
    memcpy(bufferLength,(uint8_t*)&(pduLength),2);


    char sendBuf[pduLength]; 
    sendBuf[0] = bufferLength[1];
    sendBuf[1] = bufferLength[0];
    sendBuf[2] = INITIAL_PACKET;
    sendBuf[3] = handleLen + 1; //add one because of the \0

    int sent = 0;

    for (int i = 0; i < handleLen; i++) {
        sendBuf[i + 4] = handleName[i];
    }
    sendBuf[handleLen + 4] = '\0';
    sent = sendPDU(serverSocket, sendBuf, pduLength);
    return pduLength - 4;
}


int processServer(int socketNumber) {
	char buf[MAXBUF];
	int receiveLength;
	receiveLength = recvPDU(socketNumber, buf, MAXBUF);
    int process_status = processServerPacket(socketNumber, buf);
	// if (process_status == DEFAULT_MESSAGE_FROM_SERVER) {
	// 	printf("%s\n", buf);
	// }
    return process_status;
}

void setupClientPoll(int socketNum) {
	setupPollSet();
	addToPollSet(socketNum);
	addToPollSet(STDIN_FILENO);
}

int sendInputAsPacket(int socketNum, char * buffer, int senderLength, char * senderHandle) {
    
    char * tokenSplit;
    char space[2] = " ";
    tokenSplit = strtok(buffer, space);
    int pduSize = 4 + senderLength; //initially it has this to account for Header(3bytes) + senderLength(1byte) + lengthOfSenderHandle
    PrintDollarSign();
	// printf("Buffer Message %s\n", buffer + 19);


    // MESSAGE COMMAND
    if (strcmp(tokenSplit, "%M") == 0 ) {
        pduSize = pduSize + 1; //+1 here to account for 1 byte of NUMBER OF HANDLES
        uint8_t handleAmount = atoi(strtok(NULL, space));
		
		uint8_t bufferLength[2];
        char *allHandles[handleAmount];
        char *currentHandle;
        // printf("Handle Amount is %i\n", handleAmount);
		
		int offsetMessages = handleAmount + 1 + 4; //this is the offset to account for spaces and first 4 bytes of command

        for (int i = 0; i < handleAmount; i++)
        {
            currentHandle = strtok(NULL,space);
            allHandles[i] = currentHandle;
			pduSize = pduSize + strlen(currentHandle) + 2; //add two here to account for terminator byte and length of destination byte
			offsetMessages = offsetMessages + strlen(currentHandle);
			// printf("Token: %s with length %d\n",currentHandle, strlen(currentHandle));
            // printf("Token2: %s with length2 %d\n",allHandles[i], strlen(allHandles[i]));
        }
		
		char *messageToSend = buffer + offsetMessages;
		pduSize = pduSize + strlen(messageToSend) + 1; //plus one here to account for null terminator at the end


		// printf("Message to send %s with length = %d\n", messageToSend, strlen(messageToSend));
		// printf("REMAINDER MESSAGE = %s\n", buffer+offsetMessages);

		//now create the buffer to send since we know the pdusize already
		uint8_t pduPacket[pduSize];
		memcpy(bufferLength,(uint8_t*)&(pduSize),2);
		// printf("Line 177 \n");
		uint8_t *p = pduPacket + 3; //points starting at length of sender
		// printf("Line 179 \n");
		// memcpy(pduPacket, (uint8_t)&(pduSize),2);
		// printf("Line 181 \n");
		//setting up header
		pduPacket[1] = bufferLength[0];
		pduPacket[0] = bufferLength[1];
		pduPacket[PACKET_FLAG_INDEX] = M_MESSAGE;

		// printf("Line 182\n");
		*p = senderLength;
		// printf("P sender Length %d\n", *p);
		p = p + 1; //move on to the starting of sender handle byte
		strcpy(p, senderHandle);
		// printf("P sender handle %s\n", pduPacket + 4);
		p = p + senderLength;
		*p = handleAmount;
		for (int i = 0; i < handleAmount; i++) {
			p = p + 1; //move up for destination handle length
			*p = strlen(allHandles[i]) + 1; // set destination handle length
			p = p + 1;
			strcpy(p,allHandles[i]);
			// printf("Copying allHandles %s with length of %d", allHandles[i], strlen(allHandles[i]));
			p = p + strlen(allHandles[i]); //move up for terminator byte 
			*p = '\0'; //set terminator byte
		}
		p = p + 1; //move up for beginning of sending messages
		strcpy(p,messageToSend);	
		// printf("Message to send = %s\n", p);

		// now send this created package to server
		int sent = sendPDU(socketNum, pduPacket, pduSize);
		return sent;
		// free(pduPacket);
    }


    // while (tokenSplit != NULL) {
    //     printf("%s \n", tokenSplit);
    //     tokenSplit = strtok(NULL, space);
    // }

    
    return 0;
}

int sendToServer(int socketNum, int senderLength, char *senderHandle)
{
	char stdBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */
	char exitStr[] = "exit";
    // printf("Going to readinput now \n");
	sendLen = readFromStdin(stdBuf);

    sendInputAsPacket(socketNum, stdBuf, senderLength, senderHandle);
	// printf("read: %s string len: %d (including null)\n", stdBuf, sendLen);
	
	// sent =  sendPDU(socketNum, sendBuf, sendLen);
    char listBuff[3];
    listBuff[0] = '\0';
    listBuff[1] = '\1';
    listBuff[2] = LIST_HANDLES;
    sent = sendPDU(socketNum,listBuff, 3 );
	
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	// printf("Amount of data sent is: %d\n", sent);
	// receiveLength = recvPDU(socketNum, sendBuf, sent);
	// printf("Received buffer %s\n", sendBuf);
	if (strcmp(stdBuf, exitStr) == 0) {
		return -1;
	}
	return 0;
}

int readFromStdin(char * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
    
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';

	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}

    PrintDollarSign();

	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s <handle> <server-name> <server-port> \n", argv[0]);
		exit(1);
	}
}
