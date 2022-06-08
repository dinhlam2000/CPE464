
#ifndef _SREJ_H__
#define _SREJ_H__

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
#include <stdint.h>
#include <sys/wait.h>
#include "networks.h"
#include "cpe464.h"

#define MAX_NAME 100
#define MAX_LEN 1500
#define SIZE_OF_BUF_SIZE 4
#define MAX_TRIES 10
#define LONG_TIME 10
#define SHORT_TIME 1
#define START_SEQ_NUM 0

#pragma pack (1)


typedef enum FLAG {
    FLAG_R2S_INIT = 1,
    FLAG_S2R_INIT = 2,
    FLAG_DATA = 3,
    FLAG_NONUSED = 4,
    FLAG_RR = 5,
    FLAG_SREJ = 6,
    FLAG_R2S_FNAME = 7,
    FLAG_S2R_GOOD_FNAME = 8,
    FLAG_S2R_BAD_FNAME = 9,
    FLAG_S2R_EOF = 10,
    FLAG_R2S_EOF_ACK = 11,
    FLAG_CRC_ERROR = 12
} FLAG;

int32_t send_buf(uint8_t *buf, uint32_t len, Connection *connection, 
 uint8_t flag, uint32_t seq_num, uint8_t *packet);
int32_t recv_buf(uint8_t *buf, uint32_t len, int32_t recv_sk_num, 
 Connection *connection, uint8_t *flag, uint32_t *seq_num);
int processSelect(Connection *client, int *retryCount, int selectTimeoutState, 
 int dataReadyState, int doneState);

#endif
