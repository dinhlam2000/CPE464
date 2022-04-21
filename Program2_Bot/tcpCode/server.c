
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>

#include "networks.h"
#include "cMessage.h"
#include "pollLib.h"


#define MAXBUF 1024
#define DEBUG_FLAG 1

void addNewClient(int serverSocket);
void processClient(int clientSocket);

int main(int argc, char *argv[])
{

    int serverSocket;
    int portNumber = 0;
    portNumber = checkArgs(argc,argv);
    serverSocket = tcpServerSetup(portNumber);
    serverControl(serverSocket);


    int socketReady = 0;
    while (1) {
        socketReady = poll(-1);   
        if (socketReady == serverSocket)
        {
            addNewClient(socketReady);
        }    
        else {
            processClient(socketReady);
        }
        printf("here\n");
    }


    

    return 0;
}

void processClient(int clientSocket) {
    char buf[MAXBUF];
	int messageLen = 0;
	int sent = 0;
	char exitStr[] = "exit";

	//now get the data from the client_socket
	messageLen = recvPDU(clientSocket, buf, MAXBUF);
	if (messageLen < 0)
	{
		perror("recv call");
		exit(-1);
	}
	else if (messageLen == 0) {
		perror("closed connections");
		return -1;
	}
	printf("Message received on socket %u, length: %d Data: %s\n", clientSocket, messageLen, buf);
	sent =  sendPDU(clientSocket, buf, messageLen);

	if (strcmp(buf, exitStr) == 0)
	{
		return -1;
	}
	// printf("Sent back %d\n", sent);

	return 0;
}

void addNewClient(int serverSocket) {
	int clientSocket;
	clientSocket = tcpAccept(serverSocket, DEBUG_FLAG);
	addToPollSet(clientSocket);
}

void serverControl(int serverSocket)
{
	setupPollSet();
	addToPollSet(serverSocket);
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
