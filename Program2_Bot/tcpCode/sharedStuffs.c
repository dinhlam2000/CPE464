#include "sharedStuffs.h"


int processServerPacket(int socketNumber, char *buf) {
    uint8_t flag = buf[2];
    switch (flag) {
        case POSITIVE_INITIAL_PACKET:
        {
            // printf("POSITIVE INITIAL PACKET\n");
            return POSITIVE_INITIAL_PACKET;
        }
        case NEGATIVE_INITIAL_PACKET:
        {
            // printf("NEGATIVE INITIAL PACKET\n");
            return NEGATIVE_INITIAL_PACKET;
        }
        case DEFAULT_MESSAGE_FROM_SERVER:
        {
            printf("\n%s\n", buf + 3);
            PrintDollarSign();
            return DEFAULT_MESSAGE_FROM_SERVER;
        }
        case M_MESSAGE_ERROR_DESTINATION:
        {
            printf("\nClient with handle %s does not exist\n", buf + 4);
            PrintDollarSign();
            return M_MESSAGE_ERROR_DESTINATION;   
        }
        case ACK_EXITING:
        {
            return ACK_EXITING;
        }
        case LIST_HANDLES_AMOUNT:
        {
            printf("\nNumber of clients: %i\n", buf[3]);
            return LIST_HANDLES_AMOUNT;;
        }
        case HANDLE_RETURN:
        {   
            printf("\t%s\n", buf+4);
            return HANDLE_RETURN;
        }
        case ALL_HANDLES_RETURN:
        {
            PrintDollarSign();
            return ALL_HANDLES_RETURN;
        }
        default:
        {
            return DEFAULT_MESSAGE_FROM_SERVER;
        }
    }
}

void ListOfHandlesHandler(int clientSocket, sClient **headClients) {
    sClient *p = *headClients;
    uint8_t totalClients = 0;
    
    char pduNumHandles[4];

    // Get total amount of clients
    while (p) {
        totalClients++;
        p = p->next;
    }
    pduNumHandles[0] = '\0';
    pduNumHandles[1] = '\4';
    pduNumHandles[PACKET_FLAG_INDEX] = LIST_HANDLES_AMOUNT;
    pduNumHandles[3] = totalClients;

    uint8_t bufferLength[2];
    sendPDU(clientSocket, pduNumHandles, 4);
    p = *headClients;


    while (p) {
        SendHandle(clientSocket,p->handle);
        p = p->next;
    }

    char pduFinishedSendingHandles[3];
    pduFinishedSendingHandles[0] = '\0';
    pduFinishedSendingHandles[1] = '\3';
    pduFinishedSendingHandles[PACKET_FLAG_INDEX] = ALL_HANDLES_RETURN;
    sendPDU(clientSocket, pduFinishedSendingHandles, 3);


}

void SendHandle(int clientSocket, char *handle) {
    int bufferSize = strlen(handle) + 3 + 2; //3 for header and 1 for null terminator
    char pduHandle[bufferSize];
    int bufferLength[2];
    memcpy(bufferLength, (uint8_t*)&(bufferSize),2);
    pduHandle[0] = bufferLength[1];
    pduHandle[1] = bufferLength[0];
    pduHandle[PACKET_FLAG_INDEX] = HANDLE_RETURN;
    pduHandle[3] = strlen(handle) + 1;
    strcpy(pduHandle + 4, handle);
    pduHandle[bufferSize - 1] = '\0';
    sendPDU(clientSocket, pduHandle, bufferSize);
}


void MessageHandler(int clientSocket, char * buf, sClient **headClients) {

    int lengthSender = buf[3];
    char *senderHandle = buf + 4;
    int numDestinations = buf[4 + lengthSender];

    char *destinations[numDestinations];
    char * temp = buf + 4 + lengthSender + 1;
    char *message;
    int currentDestinationHandleLength;
    for (int i = 0; i < numDestinations; i++) {
        currentDestinationHandleLength = *temp;
        destinations[i] = temp + 1;
        temp = temp + currentDestinationHandleLength + 1; //+1 to account for null terminator 
    }
    //now we got all the destinations stored in destinations
    // now temp should point at beginning of messages
    int forwardStatus;
    for (int i = 0; i < numDestinations; i ++) {
        forwardStatus = ForwardMessage(senderHandle, destinations[i], temp, headClients);
        if (forwardStatus < 0 ) {
            //this means could not find a destination so send error message
            HandleDoesNotExist(clientSocket,senderHandle, destinations[i]);
        }
    }
    // printf("done processing message buffer\n");
    
}

void HandleDoesNotExist(int senderSocket, char *senderHandle, char *destinationHandle)
{
    int bufferSize = 4 + strlen(destinationHandle) + 1; // 3 for header and 1 for length of handle and 1 for null terminator
    char sendBuf[bufferSize];
    memcpy(sendBuf,(uint8_t*)&(bufferSize),2);
    sendBuf[PACKET_FLAG_INDEX] = M_MESSAGE_ERROR_DESTINATION;
    sendBuf[3] = strlen(senderHandle) + 1;
    strcpy(sendBuf + 4, destinationHandle);
    sendBuf[bufferSize - 1] = '\0';
    sendPDU(senderSocket, sendBuf, bufferSize);
}

int ForwardMessage(char *senderHandle, char *destinationHandle, char *message, sClient **headClients) {

    sClient *p = *headClients;
    
    
    while (p) {
        //check to see if we have a handle in all clients that match with destination handle
        if (strcmp(destinationHandle,p->handle) == 0) {
            int messageLength = strlen(message) + 1;
            int bufferSize = messageLength + strlen(senderHandle) + 2 + 3; //accounts for length sender handle and 2 because of colon and space and the 3 is for header
            char sendBuf[bufferSize];
            memcpy(sendBuf,(uint8_t*)&(bufferSize),2);
            sendBuf[PACKET_FLAG_INDEX] = DEFAULT_MESSAGE_FROM_SERVER; 
            strcpy(sendBuf + 3, senderHandle);
            strcat(sendBuf + 3, ": ");
            strcat(sendBuf + 3, message);
            int sent = sendPDU(p->socket, sendBuf, bufferSize);
            return 0; //successfully sent
        }
        p = p->next;
    }
    return -1; //unable to find a destination handle with that name
}

int RemoveClient_Socket(int socketNumber, sClient **headClients) {
    sClient *p = *headClients;
    sClient *prev = NULL;

    while (p) {
        //FOUND THE CLIENT THAT NEEDS TO BE REMOVED
        if (p->socket == socketNumber) {
            if (prev == NULL) {
                p = p->next;
                *headClients = p;
                return socketNumber;
            }
            else {
                p = p->next;
                prev->next = p;
                return socketNumber;
            }
        }
        prev = p;
        p = p->next;
    }
    return -1;
}


void BroadCastHandler(int socketNumber, char * buf,sClient **headClients) {
    sClient *p = *headClients;
    char *senderHandle = buf + 4;
    char *broadCastMessage = buf + 4 + buf[3]; //this is to account for offset to get to the index of broadcast messages in the packet
    while (p) {
        if (p->socket != socketNumber)
        {
            int messageLength = strlen(broadCastMessage) + 1;   
            int bufferSize = messageLength + strlen(senderHandle) + 2 + 3; //accounts for length sender handle and 2 because of colon and space and the 3 is for header
            char sendBuf[bufferSize];
            memcpy(sendBuf,(uint8_t*)&(bufferSize),2);
            sendBuf[PACKET_FLAG_INDEX] = DEFAULT_MESSAGE_FROM_SERVER; 
            strcpy(sendBuf + 3, senderHandle);
            strcat(sendBuf + 3, ": ");
            strcat(sendBuf + 3, broadCastMessage);
            int sent = sendPDU(p->socket, sendBuf, bufferSize);
        }
        

        p = p->next;
    }   

}

int processClientPacket(int socketNumber, char * buf, sClient **headClients){


    uint8_t flag = buf[2];
    switch (flag) {
        case INITIAL_PACKET:
        {
            int status = SUCCESS_PROCESS_PACKET;
            char returnBuf[3];
            returnBuf[0] = '\0';
            returnBuf[1] = '\3'; //PDU LENGTH is 3 for returnBuf of initial packet
            returnBuf[2] = POSITIVE_INITIAL_PACKET;


            // printf("-----------Processing Inital Packet------------\n");
            char * handleName = malloc(sizeof(char) * buf[3]);
            memcpy(handleName,buf+4,buf[3]);
            if (*headClients == NULL) {
                sClient *temp = malloc(sizeof(sClient));
                temp->flag = INITIAL_PACKET;
                temp->handle = handleName;
                temp->handleLength = buf[3];
                temp->socket = socketNumber;
                temp->next = NULL;
                *headClients = temp;
            }
            else {
                int handleFound = 0;
                sClient *p = *headClients;
                
                // printf("HandleName %s\n", handleName);
                // printf("before while loop \n");
                while (p->next)  //create client
                {
                    // printf("P -> %s\n", p->handle);
                    if (strcmp(p->handle, handleName) == 0) {
                        // printf("Hanlde already in use: %s\n", handleName);
                        handleFound = 1;
                        free(handleName);
                        returnBuf[2] = NEGATIVE_INITIAL_PACKET;
                        status = ERROR_PROCESS_PACKET;
                    }
                    p = p->next;
                }
                // printf("after while loop \n");
                // check the last node as well since the while loop wouldn't check it
                if (strcmp(p->handle, handleName) == 0 && handleFound == 0) {
                    // printf("Hanlde already in use: %s\n", handleName);
                    handleFound = 1;
                    free(handleName);
                    returnBuf[2] = NEGATIVE_INITIAL_PACKET;
                    status = ERROR_PROCESS_PACKET;
                }

                // CREATE A NEW HANDLER
                if (handleFound == 0) {
                    // printf("Creating a new handle \n");
                    sClient *temp = malloc(sizeof(sClient));
                    temp->flag = INITIAL_PACKET;
                    temp->handle = handleName;
                    temp->handleLength = buf[3];
                    temp->socket = socketNumber;
                    temp->next = NULL;
                    p->next = temp;
                    // printf("Client created %s\n", p->next->handle);
                }
            }
            int returnSent = sendPDU(socketNumber, returnBuf, 3);
            // printf("Initial packet Finished!\n");
            return status;
        }
        
        case LIST_HANDLES:
        {
            ListOfHandlesHandler(socketNumber,headClients);
            return SUCCESS_PROCESS_PACKET;
        }
        case M_MESSAGE:
        {
            MessageHandler(socketNumber,buf, headClients);
            return M_MESSAGE;
        }
        case BROADCAST_PACKET:
        {
            BroadCastHandler(socketNumber, buf, headClients);
            return BROADCAST_PACKET;
        }
        case CLIENT_EXITING:
        {
            int exit_status = RemoveClient_Socket(socketNumber,headClients);
            if (exit_status == socketNumber) {
                char pduACK[3];
                pduACK[0] = '\0';
                pduACK[1] = '\3';
                pduACK[2] = ACK_EXITING;
                sendPDU(socketNumber, pduACK, 3);
                return CLIENT_EXITING;
            }
        }
        default:
            // printf("Default with flag %d\n", flag);
            return SUCCESS_PROCESS_PACKET;
    }
}