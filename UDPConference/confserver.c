//
// This program simulates a conference server that acts only a meeting point
// for a client to join/leave a conference. When there is a new client, the
// server informs its existing clients of the new client's socket address
// as well as sends the new client a list of existing clients in the conference.
// In this way, messages are sent directly between the clients. Similarly,
// when a client exits, the server informs its existing clients to adjust their
// contact list accordingly. Communication between the server and its clients
// is through UDP datagram sockets.
//
// Author: Tien Ho
// Date:   11/01/16
//

#include "utils.h"

// compare if the socket address of a client matches with a struct client
int
match(const struct client a, const struct sockaddr_in b)
{
    if ((strcmp(a.ipaddr, inet_ntoa(b.sin_addr)) == 0) && a.port == b.sin_port) {
        return 0;
    }

    return 1;
}

int
main(int argc, char **argv)
{
    int                sockfd, n, max, i;
    socklen_t          clilen, addrlen;
    struct sockaddr_in servaddr, localaddr, cliaddr, tmpaddr;
    struct client      clients[FD_SETSIZE];
    struct client      empty;
    char               recvbuff[MAXLINE], sendbuff[MAXLINE], ipaddr[MAXCHAR];


    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = 0;

    if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("error in binding");
        exit(0);
    }

    // get the local protocol address
    bzero(&localaddr, sizeof(localaddr));
    addrlen = sizeof(localaddr);
    if (getsockname(sockfd, (struct sockaddr *) &localaddr, &addrlen) < 0)
        perror("socket name error");

    printf("Started server at port %u\n", localaddr.sin_port);
    fflush(stdout);

    // set all the client entries to 0
    for (i = 0; i < FD_SETSIZE; i++) {
        bzero(&clients[i], sizeof(clients[i]));
    }

    max = -1;
    bzero(&empty, sizeof(empty));
    for ( ; ; ) {
        clilen = sizeof(cliaddr);
        bzero(recvbuff, sizeof(recvbuff));
        if ((n = recvfrom(sockfd, recvbuff, MAXLINE, 0, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
            perror("receive error");
            exit(0);
        }

        // a new client joins the conference
        if (strcmp(recvbuff, "JOIN") == 0) {
            bzero(sendbuff, sizeof(sendbuff));
            sprintf(sendbuff, "JOIN %s %u\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
            fputs(sendbuff, stdout);
            fflush(stdout);

            // relay the JOIN message along with the new client's contact
            // to all other clients
            for (i = 0; i <= max; i++) {
                if ((n = memcmp(&clients[i], &empty, sizeof(empty))) != 0) { // only sends active clients
                    bzero(&tmpaddr, sizeof(tmpaddr));
                    tmpaddr.sin_family = AF_INET;
                    tmpaddr.sin_port = clients[i].port;
                    inet_pton(AF_INET, clients[i].ipaddr, &tmpaddr.sin_addr);
                    clilen = sizeof(tmpaddr);

                    if ((n = sendto(sockfd, sendbuff, sizeof(sendbuff), 0, (struct sockaddr *) &tmpaddr, clilen)) < 0) {
                        perror("send error");
                        exit(0);
                    }
                }
            }

            // send a list of existing clients to the new client
            bzero(sendbuff, sizeof(sendbuff));
            char line[MAXLINE];
            for (i = 0; i <= max; i++) {
                if ((n = memcmp(&clients[i], &empty, sizeof(empty))) != 0) {
                    bzero(line, sizeof(line));
                    sprintf(line, "%s %u\n", clients[i].ipaddr, clients[i].port);
                    strcat(sendbuff, line);
                }
            }
            if ((n = sendto(sockfd, sendbuff, sizeof(sendbuff), 0, (struct sockaddr *) &cliaddr, clilen)) < 0) {
                perror("send error");
                exit(0);
            }

            // remember the new client
            for (i = 0; i < FD_SETSIZE; i++) {
                if ((n = memcmp(&clients[i], &empty, sizeof(empty))) == 0) {
                    strcpy(clients[i].ipaddr, inet_ntoa(cliaddr.sin_addr));
                    clients[i].port = cliaddr.sin_port;
                    break;
                }
            }

            if (i > max) {
                max = i;
            }

            if (i == FD_SETSIZE) {
                perror("too many clients");
                exit(0);
            }
        }
        else if (strcmp(recvbuff, "LEAVE") == 0) { // a client leaves the conference
            // remove the client from the list
            for (i = 0; i <= max; i++) {
                if ((n = memcmp(&clients[i], &empty, sizeof(empty))) != 0) {
                    if (match(clients[i], cliaddr) == 0) {
                        bzero(&clients[i], sizeof(clients[i]));
                        break;
                    }
                }
            }

            bzero(sendbuff, sizeof(sendbuff));
            sprintf(sendbuff, "LEAVE %s %u\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
            fputs(sendbuff, stdout);
            fflush(stdout);

            // relay the LEAVE message along with the leaving client's contact
            // to all other clients
            for (i = 0; i <= max; i++) {
                if ((n = memcmp(&clients[i], &empty, sizeof(empty))) != 0) {
                    bzero(&tmpaddr, sizeof(tmpaddr));
                    tmpaddr.sin_family = AF_INET;
                    tmpaddr.sin_port = clients[i].port;
                    inet_pton(AF_INET, clients[i].ipaddr, &tmpaddr.sin_addr);
                    clilen = sizeof(tmpaddr);

                    if ((n = sendto(sockfd, sendbuff, sizeof(sendbuff), 0, (struct sockaddr *) &tmpaddr, clilen)) < 0) {
                        perror("send error");
                        exit(0);
                    }
                }
            }
        }
    }
}
