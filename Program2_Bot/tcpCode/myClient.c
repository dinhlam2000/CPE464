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

#define MAXBUF 1024
#define DEBUG_FLAG 1

int sendToServer(int socketNum);
int readFromStdin(char * buffer);
void checkArgs(int argc, char * argv[]);
void setupClientPoll(int socketNum);
void processServer(int socketNumber);

int main(int argc, char * argv[])
{
	int socketNum = 0;         //socket descriptor
	char * pduSend;
	int socketReady = 0;
	char * pduReturned;
	
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	socketNum = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG);
	// printf("SocketNum %d\n", socketNum);
	setupClientPoll(socketNum);
	// pduSend = sendToServer(socketNum);

	

	while (1) {
		// printf("Blocking\n");
		socketReady = pollCall(-1);
		// printf("Socketready %d\n", socketReady);
		if (socketReady == socketNum)
		{
			// Receiv pdu and print it out
			// printf("Socket Readyy for server= %d \n", socketReady);
			processServer(socketReady);
		}
		else {
			// printf("Socket Readyy = %d \n", socketReady);

			pduSend = sendToServer(socketNum);
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

void processServer(int socketNumber) {
	char buf[MAXBUF];
	int receiveLength;
	receiveLength = recvPDU(socketNumber, buf, MAXBUF);
	printf("Message Received from server: %s\n", buf);

}

void setupClientPoll(int socketNum) {
	setupPollSet();
	addToPollSet(socketNum);
	addToPollSet(STDIN_FILENO);
}

int sendToServer(int socketNum)
{
	char sendBuf[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send
	int sent = 0;            //actual amount of data sent/* get the data and send it   */

	char exitStr[] = "exit";


	
	sendLen = readFromStdin(sendBuf);
	printf("read: %s string len: %d (including null)\n", sendBuf, sendLen);
	
	sent =  sendPDU(socketNum, sendBuf, sendLen);
	
	if (sent < 0)
	{
		perror("send call");
		exit(-1);
	}

	printf("Amount of data sent is: %d\n", sent);
	// receiveLength = recvPDU(socketNum, sendBuf, sent);
	// printf("Received buffer %s\n", sendBuf);
	if (strcmp(sendBuf, exitStr) == 0) {
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
	printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 3)
	{
		printf("usage: %s host-name port-number \n", argv[0]);
		exit(1);
	}
}
