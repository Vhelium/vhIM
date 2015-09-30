#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#define PORT 55099
#define HOST "vhelium.com"

int main(void)
{
    int sockfd = 0, n = 0;
    char recvBuff[1024];
    struct sockaddr_in serv_addr;
    struct hostent *server;

    memset(recvBuff, '0', sizeof(recvBuff));
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nError: couldn't create socket, lel\n");
        return 1;
    }

    server = gethostbyname(HOST);
    if (server == NULL)
    {
        printf("Error: host not found");
        return 4;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nError: connection failed \n");
        return 3;
    }

    printf("sending a message..\n");
    char *sendBuff = "This is a random message.\n";
    n = write(sockfd, sendBuff, strlen(sendBuff) + 1);
    if (n<0)
        printf("error sending.\n");
    printf("bytes sent: %d\n", n);

    printf("start listening..\n");
    while ((n = read(sockfd, recvBuff, sizeof(recvBuff)-1)) > 0)
    {
        recvBuff[n] = 0;
        if(fputs(recvBuff, stdout) == EOF)
            printf("error fputs");
    }

    if (n<0)
        printf("read error\n");
    
    return 0;
}
