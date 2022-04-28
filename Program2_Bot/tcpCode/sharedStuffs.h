#ifndef __SHAREDSTUFFS_H__
#define __SHAREDSTUFFS_H__

#include "cMessage.h"


#define MAXBUF 1400
#define SUCCESS_PROCESS_PACKET 0
#define ERROR_PROCESS_PACKET -1

#define PrintDollarSign() do {\
                            printf("$: ");\
                            fflush(stdout);\
                            } while(0)

typedef struct sClient {
    int socket;             //socket
    uint16_t handleLength;       //data length waiting to process for current packet
    uint8_t flag;           //current received flag
    char *handle;             //client name
    struct sClient *next;   //next client, NULL if only one client on the list
} sClient;

int processClientPacket(int socketNumber, char * buf, int bufferSize, sClient **headClients);
int processServerPacket(int socketNumber, char *buf);
// int PrintMessage(char *senderHandle, char *destinationHandle, char *message, sClient **headClients);
int ForwardPacket(char *senderHandle, char *destinationHandle, char *buf, int bufferSize, sClient **headClients);
void MessageHandlerClient(char * buf);
void MessageHandlerServer(int clientSocket, char * buf, int bufferSize, sClient **headClients);
void HandleDoesNotExist(int senderSocket, char *senderHandle, char *destinationHandle);
void ListOfHandlesHandler(int clientSocket, sClient **headClients);
void SendHandle(int clientSocket, char *handle);
int RemoveClient_Socket(int socketNumber, sClient **headClients);
void BroadCastHandlerClient(char * buf);
void BroadCastHandlerServer(int socketNumber, char * buf, int bufferSize, sClient **headClients);
#endif