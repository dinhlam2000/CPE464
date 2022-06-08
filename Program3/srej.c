#include "networks.h"
#include "cpe464.h"
#include "srej.h"
#include "safeUtil.h"
#include "udp_helper.h"
#include "gethostbyname.h"
int32_t send_buf(uint8_t *buf, uint32_t len, Connection *connection, uint8_t flag, uint32_t seq_num, uint8_t *packet) {
    int32_t sentLen = 0;
    int32_t sendingLen = 0;
    
    //set up the packet (seq #, crc, flag, data)
    // if (len > 0) {
    //     memcpy(&packet[sizeof(Header)], buf, len);
    // }
    sendingLen = createPDU(packet, seq_num, flag, buf, len);
    // sendingLen = createHeader(len, flag, seq_num, packet);
    // printIPInfo(&connection->remote);
    sentLen = safeSendto(connection->sk_num, packet, sendingLen, 0, (struct sockaddr *)&(connection->remote), connection->len);
    return sentLen;
}



int32_t recv_buf(uint8_t *buf, uint32_t len, int32_t recv_sk_num, Connection *connection, uint8_t *flag, uint32_t *seq_num) {
    char data_buf[MAX_LEN];
    int32_t recv_len = 0;
    // int32_t dataLen = 0;
    uint32_t remote_len = sizeof(struct sockaddr_in6);

    
    recv_len = safeRecvfrom(recv_sk_num, data_buf, len, 0, (struct sockaddr *) &(connection->remote), &remote_len);
    // recv_len = safeRecv(recv_sk_num, data_buf, len, connection);
    // dataLen = retrieveHeader(data_buf, recv_len, flag, seq_num);
    
    // receive 
    //dataLen could be -1 if crc error or if no data
    if (in_cksum(data_buf, recv_len) != 0 )
    {
        return FLAG_CRC_ERROR;
    }
    if (recv_len > 0) {
        memcpy(buf, &data_buf, recv_len);
        memcpy(seq_num, buf,4);
        *seq_num = ntohl(*seq_num);
        memcpy(flag, buf + 6, 1);
    }
    return recv_len;
}
int processSelect(Connection *client, int *retryCount, int selectTimeoutState, int dataReadyState, int doneState) {
    //Returns
    //doneState if calling this function exceeds MAX_TRIES
    //selectTimeoutState if the select times out without receiving anything
    ///dataReadyState if select() returns indicating that data is ready for read
    
    int returnVal = dataReadyState;
    
    (*retryCount)++;
    
    if (*retryCount >= MAX_TRIES) {
        printf("Sent data %d times, no ACK, client is probably gone - so I am terminating\n", MAX_TRIES);
        returnVal = doneState;
    }
    else {
        if (select_call(client->sk_num, SHORT_TIME, 0, NOT_NULL) == 1) {
            *retryCount = 0;
            returnVal = dataReadyState;
        }
        else {
            //no data ready
            returnVal = selectTimeoutState;
        }
    }
    return returnVal;
}
