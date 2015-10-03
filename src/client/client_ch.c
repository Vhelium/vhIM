#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "client_ch.h"
#include "../network/datapacket.h"
#include "../constants.h"
#include "../network/byteprocessor.h"

static int sockfd = 0;
static struct sockaddr_in serv_addr;
static struct hostent *server;

int client_ch_start(char *host, int port)
{
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nError: couldn't create socket, lel\n");
        return 1;
    }

    server = gethostbyname(host);
    if (server == NULL)
    {
        printf("Error: host not found");
        return 4;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nError: connection failed \n");
        return 3;
    }

    return 0;
}

void client_ch_send(byte *data, size_t data_len)
{
//    printf("sending a message..\n");
    char *sendBuff = (char *)data;
    ssize_t nbytes = write(sockfd, sendBuff, data_len);
    if (!nbytes)
        printf("error sending.\n");
//    printf("bytes sent: %ld\n", nbytes);
}

void client_ch_listen(callback_msg_rcv_t cb_msg_rcv)
{
    byte data_buffer[1024] = {0};
    ssize_t nbytes = 0; 
    byte rest_buffer[1024] = {0};
    size_t rest_buffer_len = 0;
    int res = 1;

    printf("start listening..\n");

    while (res && (nbytes = read(sockfd, data_buffer, sizeof(data_buffer)-1)) > 0)
    {
        res = bp_process_data(data_buffer, nbytes,
                rest_buffer, &rest_buffer_len, &sockfd, cb_msg_rcv);
    }

    if (nbytes<0)
        printf("read error\n");
    if(!res)
        printf("bullshit detected\n");
}

void client_ch_destroy()
{
    printf("destroy!?\n");
}