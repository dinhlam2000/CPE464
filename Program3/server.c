// how do we know what to put for window size, is it from client or command line

/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "udp_helper.h"
#include "cpe464.h"
#include "srej.h"
#include "window.h"


enum State {
   START, DONE, FILENAME, SEND_DATA, WAIT_ON_ACK, TIMEOUT_ON_ACK, WAIT_ON_EOF, TIMEOUT_ON_EOF
};

typedef enum State STATE;
STATE start(Connection *client, uint8_t *packet, uint32_t *seqNumSend);
STATE filename(Connection *client, uint8_t *buf, uint32_t len, uint32_t *seqNumSend, uint32_t *seqNumRecv, int32_t *data_file, int32_t *buffer_size, Window ** window);
STATE send_data(Connection *client, uint8_t *packet, int32_t *packetLength, int32_t data_file, uint32_t *seqNumSend, Window *window, int32_t bufSize);
STATE wait_on_ack(Connection *client, Window * window);
STATE wait_on_eof_ack(Connection *client);
STATE timeout_on_eof(Connection *client, Window * window, uint32_t seq_num);
STATE timeout_on_ack(Connection *client, Window * window, uint32_t seq_num);

void handleZombies(int sig);
void processClient(int socketNum, uint8_t * buf, Connection *client);
void processServer(int socketNum);
int checkArgs(int argc, char *argv[]);



int main ( int argc, char *argv[]  )
{ 
	int socketNum = 0;				
	int portNumber = 0;

	portNumber = checkArgs(argc, argv);
	sendErr_init(atof(argv[1]), DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);

		
	socketNum = udpServerSetup(portNumber);

	processServer(socketNum);

	close(socketNum);
	
	return 0;
}

void processServer(int socketNum)
{
	pid_t pid = 0;
	uint8_t buf[MAXBUF + 7];
	Connection client;
	uint8_t flag = 0;
	uint32_t seq_num = 0;
	int32_t recv_len = 0;
	int status = 1;

	// signal(SIGCHLD, handleZombies);

	while(1) 
	{
		if (select_call(socketNum, LONG_TIME, 0, SET_NULL) == 1) {
			recv_len = recv_buf(buf, MAX_LEN, socketNum, &client, &flag, &seq_num);
			client.len = sizeof(struct sockaddr_in6);
			// printf("recving %i", recv_len);
			if (recv_len != FLAG_CRC_ERROR && flag == FLAG_R2S_INIT) {
				if ((pid = fork()) < 0) { // parentf
				perror("fork");
				exit(-1);
				}
				
			if (pid == 0) { // child
				printf("child\n");
				processClient(socketNum, buf, &client);
				exit(0);
				}
			
			}

			while(waitpid(-1, &status, WNOHANG) < 0);
		}
	}
}

void processClient(int socketNum, uint8_t * buf, Connection *client)
{
	STATE state = START;
	int32_t data_file = 0;
	int32_t buffer_size = 0;
	int32_t packet_len = 0;
	uint8_t packet[MAXBUF + 7];
	uint32_t send_seq_num = START_SEQ_NUM;
	uint32_t recv_seq_num = START_SEQ_NUM;
	Window *window = NULL;
	int i = 1;


	while (i)
	{
		switch (state)
		{
			case START:
				state = start(client, packet, &send_seq_num);
				break;
			case FILENAME:
				state = filename(client, buf, MAXBUF + 7, &send_seq_num, &recv_seq_num, &data_file, &buffer_size, &window);
				break;
			case SEND_DATA:
				state = send_data(client, packet, &packet_len, data_file, &send_seq_num, window, buffer_size);
				break;
			case WAIT_ON_ACK:
				state = wait_on_ack(client, window);
				break;
			case TIMEOUT_ON_ACK:
				state = timeout_on_ack(client, window, send_seq_num - 1);
				break;
			case WAIT_ON_EOF:
				state = wait_on_eof_ack(client);
				break;
			case TIMEOUT_ON_EOF:
				state = timeout_on_eof(client, window, send_seq_num - 1);
				break;
			case DONE:
				close(client->sk_num);
				freeWindow(window);
				close(data_file);
				i = 0;
				break;
		}
	}

	// close(data_file);
}

STATE wait_on_eof_ack(Connection *client)
{
	STATE returnValue = DONE;
	uint32_t crc_check = 0;
	static int retryCount = 0;
	uint8_t buf[MAX_LEN];
	int32_t len = MAX_LEN;
	uint8_t flag = 0;
	uint32_t seq_receive = 0;

	// uint32_t rr_seq = 0;

	if ((returnValue = processSelect(client, &retryCount, TIMEOUT_ON_EOF, DONE, DONE)) == DONE)
	{
		crc_check = recv_buf(buf, len, client->sk_num, client, &flag, &seq_receive);

		if (crc_check == FLAG_CRC_ERROR)
		{
			return TIMEOUT_ON_EOF;	
		}
		else if (flag == FLAG_R2S_EOF_ACK)
		{
			return DONE;
		}
	}
	return returnValue;
}

STATE timeout_on_eof(Connection *client, Window * window, uint32_t seq_num)
{
	int index = seq_num % window->windowSize;
	Segment seg = window->Segment[index];

	// send_buf(buf, 0, client, FLAG_S2R_EOF, *seqNumSend, packet);

	safeSendto(client->sk_num, seg.pdu, seg.pduSize, 0, (struct sockaddr *)&(client->remote), client->len);
	return WAIT_ON_EOF;
}


STATE timeout_on_ack(Connection *client, Window * window, uint32_t seq_num)
{
	int index = seq_num % window->windowSize;
	Segment seg = window->Segment[index];
	// printf("timeout on ack\n");
	safeSendto(client->sk_num, seg.pdu, seg.pduSize, 0, (struct sockaddr *)&(client->remote), client->len);
	return WAIT_ON_ACK;
}

STATE wait_on_ack(Connection *client, Window * window)
{
	STATE returnValue = DONE;
	uint32_t crc_check = 0;
	static int retryCount = 0;
	uint8_t buf[MAX_LEN];
	int32_t len = MAX_LEN;
	uint8_t flag = 0;
	uint32_t seq_receive = 0;

	uint32_t rr_seq = 0;

	if ((returnValue = processSelect(client, &retryCount, TIMEOUT_ON_ACK, SEND_DATA, DONE)) == SEND_DATA)
	{
		crc_check = recv_buf(buf, len, client->sk_num, client, &flag, &seq_receive);

		if (crc_check == FLAG_CRC_ERROR)
		{
			returnValue = WAIT_ON_ACK;	
		}
		else if (flag == FLAG_R2S_EOF_ACK)
		{
			returnValue = DONE;
		}
		else if (flag == FLAG_RR || flag == FLAG_SREJ)
		{
			memcpy(&rr_seq, buf + HEADER_SIZE, 4);
			rr_seq = ntohl(rr_seq);
			if (flag == FLAG_RR)
			{
			processRR(window, rr_seq);
			}
			if (flag == FLAG_SREJ)
			{
				Segment seg = window->Segment[rr_seq % window->windowSize];
				// send_buf(buf, 0, client, FLAG_S2R_EOF, *seqNumSend, packet);
				safeSendto(client->sk_num, seg.pdu, seg.pduSize, 0, (struct sockaddr *)&(client->remote), client->len);
			}
			
		}
	}
	// printf("State = %i", returnValue);
	return returnValue;
}

STATE send_data(Connection *client, uint8_t *packet, int32_t *packetLength, int32_t data_file, uint32_t *seqNumSend, Window *window, int32_t bufSize)
{
	uint8_t buf[MAXBUF+7];
	int32_t len_read = 0;
	STATE returnValue = DONE;

	if (isClosed(window))
	{
		printf("isclosed\n");
		return WAIT_ON_ACK;
	}
	
	len_read = read(data_file, buf, bufSize);

	

	switch (len_read)
	{
		case -1:
			perror("send_data, read error");
			returnValue = DONE;
			break;
		case 0:
			(*packetLength) = send_buf(buf, 0, client, FLAG_S2R_EOF, *seqNumSend, packet);
			insertWindow(window, *seqNumSend, packet, *packetLength);
			returnValue = WAIT_ON_ACK;
			break;
		default:
			(*packetLength) = send_buf(buf, len_read, client, FLAG_DATA, *seqNumSend, packet);
			// printf("Packet Length Sent %d\n", *packetLength);
			insertWindow(window, *seqNumSend, packet, *packetLength);
			(*seqNumSend)++;
			returnValue = WAIT_ON_ACK;
	}
	return returnValue;
}
STATE filename(Connection *client, uint8_t *buf, uint32_t len, uint32_t *seqNumSend, uint32_t *seqNumRecv, int32_t *data_file, int32_t *buffer_size, Window ** window) {

	int32_t recv_len;
	uint8_t flag;
	char file_name[100];
	int32_t window_size;
	//waited for file name 
	if (select_call(client->sk_num, LONG_TIME, 0, NOT_NULL) == 0) {
      return DONE;
    }

	recv_len = recv_buf(buf, len, client->sk_num, client, &flag, seqNumRecv);
	//check for bad CRC first
	if (recv_len == FLAG_CRC_ERROR)
	{
		return DONE;
	}

	// then check if flag is file name
	if (flag == FLAG_R2S_FNAME)
	{
		memcpy(&window_size, buf + WINDOW_SIZE_OFFSET, sizeof(window_size));
		window_size = ntohl(window_size);

		memcpy(buffer_size, buf + BUFFER_SIZE_OFFSET, sizeof(*buffer_size));
		*buffer_size = ntohl(*buffer_size);

		memcpy(file_name, buf + FILENAME_OFFSET, recv_len - FILENAME_OFFSET);

		if ((*data_file = open(file_name, O_RDONLY)) < 0) {
			send_buf(NULL, 0, client, FLAG_S2R_BAD_FNAME, *seqNumSend, buf);
			return DONE;
      	}
      	else {
			send_buf(NULL, 0, client, FLAG_S2R_GOOD_FNAME, *seqNumSend, buf);
			(*seqNumSend)++;
			*window = (Window *)CreateWindow(window_size, *seqNumSend);
			return SEND_DATA;
      	}
	}

	return DONE;





}

STATE start(Connection *client, uint8_t *packet, uint32_t *seqNumSend) {
   if ((client->sk_num = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
      perror("start, open client socket");
      exit(-1);
   }
   send_buf(NULL, 0, client, FLAG_S2R_INIT, *seqNumSend, packet);
   (*seqNumSend)++;
   return FILENAME;
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

void handleZombies(int sig)
{
	int stat = 0;
	while (waitpid(-1, &stat, WNOHANG) > 0)
	{}
}


