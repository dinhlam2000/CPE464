
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
#include "sharedStuffs.h"


#define MAXBUF 1024
#define DEBUG_FLAG 1




void addNewClient(int serverSocket);
int processClient(int clientSocket, sClient **headClients);
void serverControl(int serverSocket);
int checkArgs(int argc, char *argv[]);

int main(int argc, char *argv[])
{

    int serverSocket;
    int portNumber = 0;
    portNumber = checkArgs(argc,argv);
    serverSocket = tcpServerSetup(portNumber);
    serverControl(serverSocket);

    int pduReceived;
    int socketReady = 0;
    sClient *headClients = NULL;

    while (1) {
        socketReady = pollCall(-1);   
        if (socketReady == serverSocket)
        {
            addNewClient(socketReady);
        }    
        else {
            pduReceived = processClient(socketReady, &headClients);
            if (pduReceived == -1) {
				removeFromPollSet(socketReady);
				close(socketReady);
			}
        }
    }


    

    return 0;
}

int processClient(int clientSocket, sClient **headClients) {
    char buf[MAXBUF];
	int messageLen = 0;
    int process_status;

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

    // printf("PDU LENGTH: %d\n", buf[0] | buf[1]);
    // printf("Flag: %d\n", buf[2]);
    // printf("Received: %s Message Len %d\n", buf, messageLen);

    process_status = processClientPacket(clientSocket, buf, headClients);

	
    
	return process_status;
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
