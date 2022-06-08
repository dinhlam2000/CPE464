// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "cpe464.h"

#include "udp_helper.h"
#include "window.h"
#include "srej.h"


enum State {
   START, FILENAME, FILE_OK, RECV_DATA, SEND_ACK, DONE
};

typedef enum State STATE;

int readFromStdin(char * buffer);
int checkArgs(int argc, char * argv[]);

STATE recvData(int32_t output_file_fd, Connection *server, Window *w, uint32_t *seqNumRecv, uint32_t *eofSeq);
STATE SendACK(int32_t output_file, Connection *server, Window *w, uint32_t *seqNumSend, uint32_t eof);
STATE start(char **argv, Connection *server, uint32_t *seqNumSend, int portNumber, uint32_t *seqNumRecv);
STATE fileName(char *fname, int32_t winSize, int32_t bufSize, Connection *server, uint32_t *seqNumSend, uint32_t *seqNumRecv);
STATE fileOk(int *outputFileFd, char *outputFileName);

int main (int argc, char *argv[])
 {
	// struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	int portNumber = 0; 

	Connection server;
	int32_t output_file_fd = 0;
	uint32_t seqNumSend = START_SEQ_NUM;
	uint32_t seqNumRecv = START_SEQ_NUM;
	STATE state = START;
	Window *window = NULL;
	uint32_t seqEof = 0;
	int i = 1;

	portNumber = checkArgs(argc, argv);
	sendErr_init(atof(argv[5]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

	
	// socketNum = setupUdpClientToServer(&server, argv[1], portNumber);

	while (i)
	{
		switch(state) {
			case START:
			{
				state = start(argv, &server, &seqNumSend, portNumber, &seqNumRecv);
				break;
			}
			case FILENAME:
			{
				state = fileName(argv[1], atoi(argv[3]), atoi(argv[4]), &server, &seqNumSend, &seqNumRecv);
				break;
			}
			case FILE_OK:
			{
				state = fileOk(&output_file_fd, argv[2]);
				window = (Window *)CreateWindow(atoi(argv[3]), seqNumRecv + 1);
				// window->current = seqNumRecv + 1; 
				// window->lower = seqNumRecv + 1;
				// window->upper = window->upper + seqNumRecv + 1;
				break;
			}
			case RECV_DATA:
			{
				state = recvData(output_file_fd, &server, window, &seqNumRecv, &seqEof);
				break;
			}
			case SEND_ACK:
			{
				state = SendACK(output_file_fd, &server, window, &seqNumSend, seqEof);
				break;
			}
			case DONE:
			{
				freeWindow(window);
				close(server.sk_num);
				close(output_file_fd);
				i = 0;
				break;
			}
		}
	}


	// talkToServer(socketNum, &server);
	
	// close(socketNum);

	return 0;
}

STATE recvData(int32_t output_file_fd, Connection *server, Window *w, uint32_t *seqNumRecv, uint32_t *eofSeq)
{
	STATE retVal = RECV_DATA;
	int32_t recvCheck;
	uint8_t buf[MAX_LEN];
	uint8_t flag;
	static int retryCount = 0;
	// Segment *seg = NULL;
	Segment seg;

	if ((retVal = (STATE) processSelect(server, &retryCount, 
	RECV_DATA, SEND_ACK, DONE)) == SEND_ACK) {
	
	recvCheck = recv_buf(buf, MAX_LEN, server->sk_num, server, 
	&flag, seqNumRecv);
	
		if (recvCheck != FLAG_CRC_ERROR) {
			if (flag == FLAG_DATA) {
				insertWindow(w, *seqNumRecv, buf, recvCheck);
				// printf("receiving good data: w->lower = %i w->current = %i seqNumRecv = %i \n", w->lower, w->current, *seqNumRecv);
			}

			if (flag == FLAG_S2R_EOF)
			{
				*eofSeq = *seqNumRecv;
				// printf("eofSeq = %i\n", *eofSeq);
			}
			// printf("w->lower = %i w->current = %i seqNumRecv = %i \n", w->lower, w->current, *seqNumRecv);

			// seg2 = w->Segment[w->lower % w->windowSize];
			seg = w->Segment[w->lower % w->windowSize];
			while (seg.flag)
			{
				// printf("writing ");
				// printf("w->lower = %i seqNumRecv = %i \n", w->lower, *seqNumRecv);

				write(output_file_fd, seg.pdu + 7, seg.pduSize - 7);
				w->Segment[w->lower % w->windowSize].flag = 0;
				w->lower = (w->lower + 1);
				w->upper = w->upper + 1;
				seg = w->Segment[w->lower % w->windowSize];
			}
				
		}
	}
		
	return retVal;
}

STATE SendACK(int32_t output_file, Connection *server, Window *w, uint32_t *seqNumSend, uint32_t eof) {
   STATE retVal = RECV_DATA;
   uint8_t buf[MAX_LEN], packet[MAX_LEN];
   uint8_t flag = 0;
   uint32_t seqNum;
   int32_t bufSize = sizeof(seqNum);

   if (eof == w->lower) {
      bufSize = 0;
      close(output_file);
      flag = FLAG_R2S_EOF_ACK;
      retVal = DONE;
   }
   else if (w->lower != w->current) {
      flag = FLAG_SREJ;
   }
   else {
      flag = FLAG_RR;
   }
   
   seqNum = w->lower;
   seqNum = htonl(seqNum);
   memcpy(buf, &seqNum, sizeof(seqNum));
 
   send_buf(buf, bufSize, server, flag, *seqNumSend, packet);
   (*seqNumSend)++;
   return retVal;
}


STATE start(char **argv, Connection *server, uint32_t *seqNumSend, int portNumber, uint32_t *seqNumRecv) {
	STATE retValue = START;
	uint8_t flag = 0;
	uint8_t packet[MAX_LEN];
	int32_t recvCheck = 0;

	static int retryCount = 0;

	if (server->sk_num > 0) {
        close(server->sk_num);
    }

	server->sk_num = setupUdpClientToServer((struct sockaddr_in6 *) &(server->remote), argv[6], portNumber);
	// printIPInfo(&server->remote);
	server->len = sizeof(struct sockaddr_in6);
	if (server->sk_num < 0){
		return DONE;
	}

	// if (udp_client_setup(argv[6], portNumber, server) < 0){
	// 	return DONE;
	// }
	// createPDU(pdu, sequenceNumber, 3, (uint8_t*)buffer, dataLen);
	// safeSendto(socketNum, pdu, pduLength, 0, (struct sockaddr *) server, serverAddrLen);
	send_buf(NULL, 0, server, FLAG_R2S_INIT, *seqNumSend, packet);
    
   	if ((retValue = (STATE) processSelect(server, &retryCount, 
    START, FILENAME, DONE)) == FILENAME) {
      recvCheck = recv_buf(packet, MAX_LEN, server->sk_num, server, 
       &flag, seqNumRecv); 
      if (recvCheck == FLAG_CRC_ERROR || flag != FLAG_S2R_INIT) {
	 retValue = START;
      }	
      else {
         (*seqNumSend)++;
      }
   	}
   return retValue;

}

STATE fileName(char *fname, int32_t winSize, int32_t bufSize, Connection *server, uint32_t *seqNumSend, uint32_t *seqNumRecv)
 {
	STATE returnValue = START;
	uint8_t packet[MAX_LEN], buf[MAX_LEN];
	uint8_t flag = 0;
	int32_t fname_len = strlen(fname) + 1;
	int32_t crc_check = 0;
	static int retryCount = 0;
		
	winSize = htonl(winSize);
	memcpy(buf, &winSize, sizeof(winSize));
	bufSize = htonl(bufSize);
	memcpy(buf + sizeof(winSize), &bufSize, sizeof(bufSize));
	memcpy(buf + sizeof(winSize) + sizeof(bufSize), fname, fname_len);
	send_buf(buf, fname_len + sizeof(winSize) + sizeof(bufSize), server, 
    FLAG_R2S_FNAME, *seqNumSend, packet);

	if ((returnValue = (STATE)processSelect(server, &retryCount, START, FILE_OK, DONE)) == FILE_OK)
	{
		crc_check = recv_buf(packet, MAX_LEN, server->sk_num, server, &flag, seqNumRecv);
		if (crc_check == FLAG_CRC_ERROR)
		{
			return START;
		}
		else if (flag == FLAG_S2R_BAD_FNAME)
		{
			return DONE;
		}
		else if (flag == FLAG_S2R_GOOD_FNAME)
		{
			(*seqNumSend)++;
		}
		else {
			return FILENAME;
		}
	}
	return returnValue;
	
 }

STATE fileOk(int *outputFileFd, char *outputFileName)
{
	STATE retVal = RECV_DATA;
	
   if ((*outputFileFd = open(outputFileName, 
    O_CREAT | O_TRUNC | O_WRONLY, 0600)) < 0) {
      perror("File open error: ");
      retVal = DONE;
   }
   return retVal;
}
int readFromStdin(char * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

int checkArgs(int argc, char * argv[])
{

	int portNumber = 0;
	
        /* check command line arguments  */
	if (argc != 8)
	{
		printf("usage: %s from-filename to-filename window-size buffer-size error-percent remote-machine remote-port\n", argv[0]);
		exit(1);
	}
	if (strlen(argv[1]) > 100) {
      printf("Local filename must be < 100\n");
      exit(-1);
	}
	
   if (strlen(argv[2]) > 100) {
      printf("Remote filename must be < 100\n");
      exit(-1);
   }
	
   if (atoi(argv[3]) < 1) {
      printf("Invalid window size\n");
      exit(-1);
   }
	
   if (atoi(argv[4]) < 400 || atoi(argv[4]) > 1400) {
      printf("buffer size must be between 400 and 1400\n");
      exit(-1);
   }
	
   if (atof(argv[5]) < 0 || atof(argv[5]) > 1) {
      printf("Error rate must be between 0 and 1\n");
      exit(-1);
   }	

	
	portNumber = atoi(argv[7]);
		
	return portNumber;
}





