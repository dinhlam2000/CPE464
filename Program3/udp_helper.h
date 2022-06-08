#ifndef __UDPHEADER_H__
#define __UDPHEADER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int createPDU(uint8_t * pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int payloadLen);
void printPDU(uint8_t * pduBuffer, int pduLength);
#endif
