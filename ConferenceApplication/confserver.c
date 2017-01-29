//
// This program simulates a conference server that enables a group communication
// between multiple clients. When the server receives a message from any of its
// conference clients, it relays the message to all other conference clients.
//
// Author: Tien Ho
// Date:   10/06/16
//

#include "utils.h"

int
main(int argc, char **argv)
{
    int                listenfd, connfd, i, j, maxfd, nready, max, n, currentcli;
    char               recvbuff[MAXLINE], sendbuff[MAXLINE], cliname[MAXLINE], message[MAXLINE];
    struct sockaddr_in servaddr, cliaddr, localaddr;
    socklen_t          clilen, addrlen;
    int                client[FD_SETSIZE];
    fd_set             rset, allset;

    // create a listen socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = 0;

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("error in binding");
        exit(0);
    }

    // get the local protocol address
    bzero(&localaddr, sizeof(localaddr));
    addrlen = sizeof(localaddr);
    if (getsockname(listenfd, (struct sockaddr *) &localaddr, &addrlen) < 0)
        perror("socket name error");

    printf("Started server at port %u\n", localaddr.sin_port);
    fflush(stdout);

    if (listen(listenfd, LISTENQ) < 0)
        exit(0);

    // set all the entries in the client array to -1 => no connecting clients
    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;

    maxfd = listenfd;
    max = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    for ( ; ; ) {
        rset = allset; // reset the readset for select
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        clilen = sizeof(cliaddr);
        // when there is a new connection request
        if (FD_ISSET(listenfd, &rset)) {
            if ((connfd = accept(listenfd, NULL, NULL)) < 0) {
                perror("connection error");
                continue;
            }

            // get the protocol address of the connected peer socket
            bzero(&cliaddr, sizeof(cliaddr));
            clilen = sizeof(cliaddr);
            if (getpeername(connfd, (struct sockaddr *) &cliaddr, &clilen) < 0)
                perror("socket name error");

            bzero(message, sizeof(message));
            sprintf(message, "Server: connect from \'%s\' at port \'%u\'\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
            fputs(message, stdout);
            fflush(stdout);

            // save the client descriptor
            for (i = 0; i < FD_SETSIZE; i++) {
                if (client[i] < 0) {
                    client[i] = connfd;
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                perror("too many clients");
                exit(0);
            }

            // add this new descriptor for future select
            FD_SET(connfd, &allset);
            if (connfd > maxfd) {
                maxfd = connfd;
            }
            if (i > max) {
                max = i;
            }
            if (--nready <= 0) {
                continue; // no more readable descriptors
            }
        }

        for (i = 0; i <= max; i++) {
            currentcli = client[i];
            if (currentcli > -1) {
                // socket is readable
                if (FD_ISSET(currentcli, &rset)) {
                    clilen = sizeof(cliaddr);
                    if (getpeername(currentcli, (struct sockaddr *) &cliaddr, &clilen) < 0)
                        perror("socket name error");

                    bzero(recvbuff, sizeof(recvbuff));
                    if ((n = read(currentcli, recvbuff, MAXLINE)) == 0) { // the client terminates the connection
                        FD_CLR(currentcli, &allset);
                        close(currentcli);
                        client[i] = -1;

                        bzero(message, sizeof(message));
                        sprintf(message, "Server: disconnect from \'%s\'(%u)\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
                        fputs(message, stdout);
                        fflush(stdout);
                    }
                    else if (n > 0) {
                        // broadcast the message to all other clients
                        bzero(sendbuff, sizeof(sendbuff));
                        bzero(cliname, sizeof(cliname));
                        sprintf(cliname, "\'%s\'(%u): ", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
                        strcpy(sendbuff, cliname);
                        strcat(sendbuff, recvbuff);
                        fputs(sendbuff, stdout);
                        fflush(stdout);
                        for (j = 0; j <= max; j++) {
                            if (client[j] > -1 && client[j] != currentcli) {
                                if(write(client[j], sendbuff, strlen(sendbuff)) < 0) {
                                    perror("write error");
                                    exit(0);
                                }
                            }
                        }
                    }
                    else {
                        perror("read error");
                        exit(0);
                    }

                    if (--nready <= 0) {
                        break; // no more readable descriptors
                    }
                }
            }
        }
    }
}
