#include "udp_helper.h"
#include "checksum.h"

int createPDU(uint8_t * pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int payloadLen) {



    uint32_t temp = htonl(sequenceNumber);
    uint16_t zero = 0;
    //zero out the 2 byte cksum
    memset(pduBuffer+4,0,2);
    // 4 byte sequence # in network order
    memcpy(pduBuffer,&(temp),4);
    // copy flag into pdu
    memcpy(pduBuffer+6,&(flag), 1);
    // copy payload into pdu
    memcpy(pduBuffer+7, payload, payloadLen);

    
    // calculate crc and put it in pdu
    uint16_t crc = in_cksum(pduBuffer, payloadLen + 7);
    memcpy(pduBuffer+4,&(crc),2);
    return payloadLen + 7;
    
}

void printPDU(uint8_t * pduBuffer, int pduLength) {
    uint32_t sequenceNumber;
    memcpy(&sequenceNumber, pduBuffer,4);
    sequenceNumber = ntohl(sequenceNumber);
    uint16_t crc;
    memcpy(&crc, pduBuffer+4,2);
    uint8_t flag;
    memcpy(&flag, pduBuffer+6,1);
    // uint8_t payloacre
    printf("Sequence Number: %u\n", sequenceNumber);
    printf("Checksum: %u\n", crc);
    printf("Flag: %u\n", flag);
    printf("Payload: %s\n", pduBuffer + 7);
}
