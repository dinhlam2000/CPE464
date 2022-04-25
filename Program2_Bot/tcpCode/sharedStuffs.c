#include "sharedStuffs.h"

void SendPacket(u_int8_t flag) {
    switch (flag) {
        case INITIAL_PACKET:
            printf("Initial Packet\n");
        default:
            printf("Default\n");
    }
}

int processServerPacket(int socketNumber, char *buf) {
    uint8_t flag = buf[2];
    switch (flag) {
        case POSITIVE_INITIAL_PACKET:
        {
            printf("POSITIVE INITIAL PACKET\n");
            return POSITIVE_INITIAL_PACKET;
        }
        case NEGATIVE_INITIAL_PACKET:
        {
            printf("NEGATIVE INITIAL PACKET\n");
            return NEGATIVE_INITIAL_PACKET;
        }
        case DEFAULT_MESSAGE_FROM_SERVER:
        {
            printf("%s\n", buf + 3);
        }
        default:
        {
            return DEFAULT_MESSAGE_FROM_SERVER;
        }
    }
}


void MessageHandler(char * buf, sClient **headClients) {

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
            printf("CLIENT %s does not exist\n", destinations[i]);
        }
    }
    printf("done processing message buffer\n");
    
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


            printf("-----------Processing Inital Packet------------\n");
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
                
                printf("HandleName %s\n", handleName);
                printf("before while loop \n");
                while (p->next)  //create client
                {
                    printf("P -> %s\n", p->handle);
                    if (strcmp(p->handle, handleName) == 0) {
                        printf("Client already existed. Choose a different handle name\n");
                        handleFound = 1;
                        free(handleName);
                        returnBuf[2] = NEGATIVE_INITIAL_PACKET;
                        status = ERROR_PROCESS_PACKET;
                    }
                    p = p->next;
                }
                printf("after while loop \n");
                // check the last node as well since the while loop wouldn't check it
                if (strcmp(p->handle, handleName) == 0 && handleFound == 0) {
                    printf("Client already existed. Choose a different handle name\n");
                    handleFound = 1;
                    free(handleName);
                    returnBuf[2] = NEGATIVE_INITIAL_PACKET;
                    status = ERROR_PROCESS_PACKET;
                }

                // CREATE A NEW HANDLER
                if (handleFound == 0) {
                    printf("Creating a new handle \n");
                    sClient *temp = malloc(sizeof(sClient));
                    temp->flag = INITIAL_PACKET;
                    temp->handle = handleName;
                    temp->handleLength = buf[3];
                    temp->socket = socketNumber;
                    temp->next = NULL;
                    p->next = temp;
                    printf("Client created %s\n", p->next->handle);
                }
            }
            int returnSent = sendPDU(socketNumber, returnBuf, 3);
            printf("Initial packet Finished!\n");
            return status;
        }
        
        case LIST_HANDLES:
        {
            printf("----------Listing Handles---------\n");
            sClient *p = *headClients;
            while (p){
                printf("P->Handle: %s\n", p->handle);
                p = p->next;
            }
            return SUCCESS_PROCESS_PACKET;
        }
        case M_MESSAGE:
        {
            MessageHandler(buf, headClients);
        }
        default:
            printf("Default with flag %d\n", flag);
            return SUCCESS_PROCESS_PACKET;
    }
}