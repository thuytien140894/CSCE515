//
// This program simulates a conference client that connects to a conference
// server to join a group communication with other conference clients. The client
// can read the message from the user input and send it to the server. Also,
// the client outputs the message received from the server.
//
// Author: Tien Ho
// Date:   10/06/16
//

#include "utils.h"

int
main(int argc, char **argv)
{
    int                sockfd, maxfd, n;
    socklen_t          addrlen;
    struct sockaddr_in servaddr, localaddr;
    fd_set             rset, allset;
    char               sendbuff[MAXLINE], recvbuff[MAXLINE], welcome[MAXLINE];

    if (argc != 3) {
        perror("usage: confclient <servhost> <servport>");
        exit(0);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = (uint16_t)atoi(argv[2]);

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        printf("inet_pton error for %s", argv[1]); // argv[1] == IP address
        exit(0);
    }

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(0);
    }

    // get the local protocol address
    bzero(&localaddr, sizeof(localaddr));
    addrlen = sizeof(localaddr);
    if (getsockname(sockfd, (struct sockaddr *) &localaddr, &addrlen) < 0)
        perror("socket name error");

    bzero(welcome, sizeof(welcome));
    sprintf(welcome, "Connected to server on \'%s\' at port \'%s\' through port \'%u\'\n",
            argv[1], argv[2], localaddr.sin_port);
    fputs(welcome, stdout);
    fflush(stdout);

    FD_ZERO(&allset);
    FD_SET(fileno(stdin), &allset);
    FD_SET(sockfd, &allset);
    maxfd = max(sockfd, fileno(stdin));
    for ( ; ; ) {
        rset = allset; // reset the rset for select
        select(maxfd + 1, &rset, NULL, NULL, NULL);

        // socket is readable
        if (FD_ISSET(sockfd, &rset)) {
            bzero(recvbuff, sizeof(recvbuff));
            if ((n = read(sockfd, recvbuff, MAXLINE)) == 0) { // the server terminated the connection
                perror("server terminated prematurely");
                exit(0);
            }
            else if (n > 0) {
                fputs(recvbuff, stdout);
            }
            else {
                perror("read error");
            }
        }

        // standard input is readable
        if (FD_ISSET(fileno(stdin), &rset)) {
            bzero(sendbuff, sizeof(sendbuff));
            if (fgets(sendbuff, MAXLINE, stdin) != NULL) {
                if (write(sockfd, sendbuff, strlen(sendbuff)) < 0) {
                    perror("write error");
                    exit(0);
                }
            }
        }
    }
}
