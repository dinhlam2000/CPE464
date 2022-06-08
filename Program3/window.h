#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <stdlib.h>
#include <stdint.h>
// #include "srej.h"
#define MAXBUF 1400

#define HEADER_SIZE 7
#define WINDOW_SIZE_OFFSET 7
#define BUFFER_SIZE_OFFSET 11
#define FILENAME_OFFSET 15

typedef struct {
   
   uint32_t pduSize;
   uint32_t seqNumber;
   uint32_t flag;
   uint8_t pdu[MAXBUF + 7];
} __attribute__((packed)) Segment;

typedef struct {
   Segment * Segment;
   uint32_t windowSize;
   uint32_t lower;
   uint32_t upper;
   uint32_t current;   

} Window;

Window* CreateWindow(int32_t winSize, uint32_t seqNum);
void insertWindow(Window * window, uint32_t seqNum, uint8_t *pdu, uint32_t pduSize);
void processRR(Window * window, uint32_t ackSeqNum);
void freeWindow(void *window);

void PrintWindow(Window * window);
void PrintMetaDataWindow(Window * window);
int isClosed(Window * window);
int checkSeqNumber(Window * window, uint32_t seqNumber);
#endif