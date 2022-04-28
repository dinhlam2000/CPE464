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
#include <ctype.h>

#include "networks.h"
#include "cMessage.h"
#include "pollLib.h"
#include "sharedStuffs.h"


#define DEBUG_FLAG 1


int sendToServer(int socketNum, int senderLength, char *senderHandle);
int readFromStdin(char * buffer);
void checkArgs(int argc, char * argv[]);
void setupClientPoll(int socketNum);
int processServer(int socketNumber);
int sendInitialPacket(char * handleName, int serverSocket);
int sendInputAsPacket(int socketNum, char * buffer, int senderLength, char *senderHandle);

int main(int argc, char * argv[])
{
    checkArgs(argc, argv);

	int socketNum = 0;         //socket descriptor
	int pduSend;
	int socketReady = 0;
	int pduReturned;

	int senderLength = strlen(argv[1]);
	if (isdigit(argv[1][0])) {
		printf("Invalid handle, handle starts with a number\n");
		exit(-1);
	}
 	else if (senderLength > 100) {
		printf("Invalid handle, handle longer than 100 characters: %s\n", argv[1]);
		exit(-1);
	}


    char *clientHandleName = malloc(sizeof(char) * (senderLength + 1)); //add one to take account for that '\0'
    memcpy(clientHandleName,argv[1],senderLength);
	senderLength++; //accounts for null terminator as a singular byte
    clientHandleName[senderLength] = '\0';
    // printf("ClientHandleName %s\n", clientHandleName);
    


	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);
	// printf("SocketNum %d\n", socketNum);
	setupClientPoll(socketNum);
    sendInitialPacket(argv[1], socketNum);
	// pduSend = sendToServer(socketNum);

	PrintDollarSign();

	while (1) {
		// printf("Blocking\n");
		socketReady = pollCall(-1);
		// printf("Socketready %d\n", socketReady);
		if (socketReady == socketNum)
		{
			// Receiv pdu and print it out
			pduReturned = processServer(socketReady);

			// printf("FLAG RETURNED FROM SERVER %d", pduReturned);
            if ((pduReturned == NEGATIVE_INITIAL_PACKET)) {
				printf("Handle already in use: %s\n", clientHandleName);
                break;
            }
			else if (pduReturned == ACK_EXITING) {
				break;
			}
			else if (pduReturned == -1) {
				printf("Server Terminated\n");
				break;
			}
		}
		else {
			// printf("Socket Readyy = %d \n", socketReady);
			// fprintf(0, "$: ");
			

			pduSend = sendToServer(socketNum, senderLength, clientHandleName);
			PrintDollarSign();
			if (pduSend < 0) {
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

    // uint8_t bufferLength [2];
    int handleLen= strlen(handleName);
    int pduLength = handleLen + 3; //first extra 4 byte is for the packet set up and last byte is \0
    // memcpy(bufferLength,(uint8_t*)&(pduLength),2);


    char sendBuf[pduLength]; 
    // sendBuf[0] = bufferLength[1];
    // sendBuf[1] = bufferLength[0];
    sendBuf[0] = INITIAL_PACKET;
    sendBuf[1] = handleLen + 1; //add one because of the \0

    int sent = 0;

    for (int i = 0; i < handleLen; i++) {
        sendBuf[i + 2] = handleName[i];
    }
    sendBuf[handleLen + 2] = '\0';
    sent = sendPDU(serverSocket, sendBuf, pduLength);
    return sent;
}


int processServer(int socketNumber) {
	char buf[MAXBUF];
	int receiveLength;
	receiveLength = recvPDU(socketNumber, buf, MAXBUF);
	if (receiveLength == 0) {
		//server terminated
		return -1;
	}
	// printf("Received Buffer %s\n", buf);
    int process_status = processServerPacket(socketNumber, buf);
	// printf("Process status %i\n", process_status);
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
    int pduSize;
	pduSize = 2 + senderLength; //initially it has this to account for Header(3bytes) + senderLength(1byte) + lengthOfSenderHandle
    // PrintDollarSign();
	// printf("Buffer Message %s\n", buffer + 19);


    // MESSAGE COMMAND
    if ((strcmp(tokenSplit, "%M") == 0) || strcmp(tokenSplit, "%m") == 0) {
        pduSize = pduSize + 1; //+1 here to account for 1 byte of NUMBER OF HANDLES
		char *handleInput = strtok(NULL,space);
		if (!handleInput) {
			printf("Invalid command format\n");
			return 0;
		}
        int handleAmount = atoi(handleInput);
		if (!handleAmount) {
			printf("Invalid command format\n");
			return 0;
		}
		if (handleAmount > 9) {
			printf("Number of handles must be between 1-9\n");
			return 0;
		}
		
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
		// printf("message to send length %u\n", strlen(messageToSend));
		pduSize = pduSize + strlen(messageToSend) + 1; //plus one here to account for null terminator at the end


		// printf("Message to send %s with length = %d\n", messageToSend, strlen(messageToSend));
		// printf("REMAINDER MESSAGE = %s\n", buffer+offsetMessages);

		//now create the buffer to send since we know the pdusize already
		uint8_t pduPacket[pduSize];
		// printf("Line 177 \n");
		uint8_t *p = pduPacket + 1; //points starting at length of sender
		// printf("Line 179 \n");
		// memcpy(pduPacket, (uint8_t)&(pduSize),2);
		// printf("Line 181 \n");
		//setting up header
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

	else if ( (strcmp(tokenSplit, "%B") == 0) || (strcmp(tokenSplit, "%b") == 0) ) {
		char * messageToSend = buffer + 3;
		int messageLength = strlen(messageToSend);
		int breakup= 0;
		//have to break up in multiple packets
		if (messageLength > 200)
		{
			breakup = messageLength / 200;
			messageLength = 200;
			// printf("Break up %i\n", breakup);
		}
		pduSize = pduSize + messageLength + 1; //1 for null terminator
		uint8_t pduPacket[pduSize];
		// uint8_t bufferLength[2];
		// memcpy(bufferLength, (uint8_t*)&(pduSize), 2);
		// pduPacket[0] = bufferLength[1];
		// pduPacket[1] = bufferLength[0];
		pduPacket[PACKET_FLAG_INDEX] = BROADCAST_PACKET;
		pduPacket[1] = senderLength;
		strcpy(pduPacket + 2, senderHandle);
		int sent = 0;
		if (breakup > 0)
		{
			for (int i = 0; i <= breakup; i++) 
			{
				if (i != breakup)
				{
					strncpy(pduPacket + 2 + senderLength, messageToSend, 200);
					pduPacket[pduSize - 1] = '\0';
					sent += sendPDU(socketNum, pduPacket, pduSize);
					messageToSend = messageToSend + 200;
				}
				else {
					strcpy(pduPacket + 2 + senderLength, messageToSend);
					pduSize = pduSize - (200 - strlen(messageToSend)) + 1; //extra 1 byte for terminator again
					pduPacket[pduSize - 1] = '\0';
					// printf("")
					sent += sendPDU(socketNum, pduPacket, pduSize);
				}
			}
		}
		else {
			strcpy(pduPacket + 2 + senderLength, messageToSend);
			pduPacket[pduSize - 1] = '\0';
			sent += sendPDU(socketNum, pduPacket, pduSize);
		}
		
		return sent;

	}
	else if ((strcmp(tokenSplit, "%L") == 0) || (strcmp(tokenSplit, "%l") == 0)){
		uint8_t pduPacket[1];
		int pduLength = 1;
		// memcpy(pduPacket,(uint8_t*)&(pduLength),2);
		pduPacket[PACKET_FLAG_INDEX] = LIST_HANDLES;
		int sent = sendPDU(socketNum,pduPacket,pduLength);
		return sent;
	}
	else if ((strcmp(tokenSplit, "%E") == 0) || (strcmp(tokenSplit, "%e") == 0)) {
		uint8_t pduPacket[1];
		int pduLength = 1;
		// memcpy(pduPacket, (uint8_t*)&(pduLength),2);
		pduPacket[PACKET_FLAG_INDEX] = CLIENT_EXITING;
		int sent = sendPDU(socketNum, pduPacket, pduLength);
		return sent;
	}
	else{
		printf("Invalid command\n");
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
    // printf("Going to readinput now \n");
	sendLen = readFromStdin(stdBuf);
	if (sendLen > MAXBUF) {
		printf("Too long of an input. Max is 1400 characters including (command, handles and text)\n");
		return 0;
	}

    sendInputAsPacket(socketNum, stdBuf, senderLength, senderHandle);
	// printf("read: %s string len: %d (including null)\n", stdBuf, sendLen);
	
	// sent =  sendPDU(socketNum, sendBuf, sendLen);
    // char listBuff[3];
    // listBuff[0] = '\0';
    // listBuff[1] = '\1';
    // listBuff[2] = LIST_HANDLES;
    // sent = sendPDU(socketNum,listBuff, 3 );
	
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	// printf("Amount of data sent is: %d\n", sent);
	// receiveLength = recvPDU(socketNum, sendBuf, sent);
	// printf("Received buffer %s\n", sendBuf);
	
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

    // PrintDollarSign();

	
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
