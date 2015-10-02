#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../constants.h"
#include "../network/datapacket.h"
#include "../network/byteprocessor.h"

static fd_set master;      // master file descriptor list
static fd_set read_fds;    // temp file descriptor list for select()
static int fdmax;          // maximum file descriptor number

static int listener;       // listening socket descriptor

static void *get_in_addr(struct sockaddr *sa)
{
    if(sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int server_ch_start(char *port)
{
    int rv;

    int yes=1;          // for setsockopt()
    struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);   // clear master and temp sets
    FD_ZERO(&read_fds);

    // get the socket and bind it
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(listener < 0)
            continue;

        // ignore "adress already in use" spam
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if(bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }

        break;
    }

    // check if socket is bound
    if(p == NULL)
    {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }

    freeaddrinfo(ai); // no need for it again

    // listen
    if(listen(listener, 10) == -1)
    {
        perror("listen");
        exit(3);
    }

    // add listener to master set
    FD_SET(listener, &master);

    // keep track of biggest file descriptor'
    fdmax = listener; //currently it's this one

    return 0;
}

void server_ch_send(int fd, byte *data, size_t data_len)
{
    if (send(fd, data, data_len, 0) == -1)
        perror("send");
}

void server_ch_send_all(int fd_from, byte *data, size_t data_len)
{
    int j;
    for(j=0; j <= fdmax; ++j)
    {
        // send it to everyone, yayy
        if (FD_ISSET(j, &master))
        {
            // except the listener (and ourselves)
            if (j != listener && j != fd_from)
                if (send(j, data, data_len, 0) == -1)
                    perror("send");
        }
    }
}

void server_ch_listen(void (*cb_cl_cntd)(int fd),
        void (*cb_msg_rcv)(int fd, byte *data),
        void (*cb_cl_dc)(int fd))
{
    byte data_buffer[1024] = {0};
    size_t nbytes = 0; 
    byte rest_buffer[1024] = {0};
    size_t rest_buffer_len = 0;
    int res = 1;

    int newfd;                          // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; //client address
    socklen_t addrlen;

//    char remoteIP[INET6_ADDRSTRLEN];

    int i;

    // main loop
    for(;;)
    {
        read_fds = master;  // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }

        // run through existing connections looking for data to read
        for(i = 0; i <= fdmax; ++i)
            if(FD_ISSET(i, &read_fds)) // found sth
            {
                if (i == listener)
                {
                    // handle new connections
                    addrlen = sizeof(remoteaddr);
                    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

                    if (newfd == -1)
                        perror("accept");
                    else
                    {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax)      // keep track of max
                            fdmax = newfd;

                        // inform server
                        cb_cl_cntd(newfd);

//                        printf("selectserver: new connection from %s on socket %d\n",
//                                inet_ntop(remoteaddr.ss_family,
//                                    get_in_addr((struct sockaddr*)&remoteaddr),
//                                    remoteIP, INET6_ADDRSTRLEN), newfd);
                    }
                }
                else
                {
                    // handle data from a client
                    if ((nbytes = recv(i, data_buffer, sizeof(data_buffer), 0)) <= 0)
                    {
                        // got error or connection closed by client
                        if (nbytes == 0)
                        {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        }
                        else
                            perror("recv");
                        close(i);
                        FD_CLR(i, &master); // remove from master set
                    }
                    else
                    {
                        printf("bytes received: %ld\n", nbytes);
                        res = bp_process_data(data_buffer, nbytes,
                                rest_buffer, &rest_buffer_len, i, cb_msg_rcv);

                        if (!res)
                            perror("processing data");

//                        // we got some data from a client
//                        for(j=0; j <= fdmax; ++j)
//                        {
//                            // send it to everyone, yayy
//                            if (FD_ISSET(j, &master))
//                            {
//                                // except the listener (and ourselves)
//                                if (j != listener && j != i)
//                                    if (send(j, data_buffer, nbytes, 0) == -1)
//                                        perror("send");
//                            }
//                        }
                    }
                }
            }
    }// end for(;;)
}

void server_ch_destroy()
{
}
