/******************************************************************************
* tcp_server.c
*
* CPE 464 - Program 1
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

#define MAXBUF 1024
#define DEBUG_FLAG 1

char * recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
int main(int argc, char *argv[])
{
	int serverSocket = 0;   //socket descriptor for the server socket
	int clientSocket = 0;   //socket descriptor for the client socket
	int portNumber = 0;
	char * pduReceived;
	char exit[] = "exit";
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	serverSocket = tcpServerSetup(portNumber);

	// wait for client to connect


	while (1) {
		clientSocket = tcpAccept(serverSocket, DEBUG_FLAG);
		pduReceived = recvFromClient(clientSocket);
		while (strcmp(pduReceived, exit) != 0) {
			pduReceived = recvFromClient(clientSocket);
		}
		close(clientSocket);
	}
	
	/* close the sockets */
	
	close(serverSocket);

	return 0;

}
char * recvFromClient(int clientSocket)
{
	char buf[MAXBUF];
	int messageLen = 0;
	int sent = 0;
	//now get the data from the client_socket
	messageLen = recvPDU(clientSocket, buf, MAXBUF);
	if (messageLen < 0)
	{
		perror("recv call");
		exit(-1);
	}
	else if (messageLen == 0) {
		perror("closed connections");
		return "exit";
	}
	printf("Message received on socket %u, length: %d Data: %s\n", clientSocket, messageLen, buf);
	sent =  sendPDU(clientSocket, buf, messageLen);
	printf("Sent back %d\n", sent);
	return buf;
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	return portNumber;
}

