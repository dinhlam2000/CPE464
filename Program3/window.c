#include <string.h>
#include <stdio.h>
#include "window.h"


Window* CreateWindow(int32_t winSize, uint32_t seqNum) {
   Window *window = (Window *) calloc(1,sizeof(Window));
   Segment *segment = (Segment *) calloc(winSize, sizeof(Segment));
   window->Segment = segment;
   window->windowSize = winSize;
   window->lower = seqNum;
   window->current = seqNum;
   window->upper = window->lower + winSize;
   // window->currentOpen = winSize;
   return window;
}

void insertWindow(Window * window, uint32_t seqNum, uint8_t *pdu, uint32_t pduSize) {
   int index = seqNum % window->windowSize;
   // printf("seqNum = %i lower = %i upper = %i current = %i\n", seqNum, window->lower, window->upper, window->current);

   if (!isClosed(window) && checkSeqNumber(window,seqNum) && window->Segment[index].flag != 1) {
      window->Segment[index].pduSize = pduSize;
      window->Segment[index].seqNumber = seqNum;
      window->Segment[index].flag = 1;
      memcpy(window->Segment[index].pdu, pdu, pduSize);
      // window->currentOpen--; // decrement 1 because now 1 segment is taken
      window->current += 1;  
   }
}

void processRR(Window * window, uint32_t ackSeqNum) {
   while (window->lower < ackSeqNum) {
      (window->Segment + (window->lower % window->windowSize))->flag = 0;
      window->upper++; 
      window->lower++;
      // window->currentOpen++; // increment one because now 1 slot is open
   }
}

int checkSeqNumber(Window * window, uint32_t seqNumber) {
   if (seqNumber >= window->lower && seqNumber < window->upper){
      return 1;
   }
   return 0;
}


int isClosed(Window * window) {
   if (window->current == window->upper) {
      return 1;
   }
   return 0;
}

void PrintMetaDataWindow(Window * window){
   printf("Server Window - Window Size: %i , lower: %i, Upper: %i, Current: %i window open?: %i\n", window->windowSize, window->lower, window->upper, window->current, window->current == window->upper ? 0 : 1);
}

void PrintWindow(Window * window) {
   printf("Window size is:%i\n", window->windowSize);
   int i = 0;
   for ( i; i < window->windowSize; i++) {
      if (window->Segment[i].flag == 1)
      {
         printf("\t%i sequenceNumber: %i pduSize: %i\n", i, window->Segment[i].seqNumber, window->Segment[i].pduSize);
      }
      else {
         printf("\t%i not valid\n", i);
      }
   }
}

void freeWindow(void *window) {
   Window *w = (Window *) window;                                   
   free(w->Segment);                                       
   free(w);
}
