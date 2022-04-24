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
        default:
        {
            return -1;
        }
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
        default:
            printf("Default with flag %d\n", flag);
            return SUCCESS_PROCESS_PACKET;
    }
}