//
// This program simulates a conference client that contacts a conference
// server using UDP to join a group communication with other conference clients.
// The client then sends messages directly to all other clients using the list
// of contacts it receives from the server.
//
// Author: Tien Ho
// Date:   11/01/16
//
#include "utils.h"

// global variables to be accessed by the signal handler
char               sendbuff[MAXLINE];
struct sockaddr_in servaddr;
int                sockfd, n;

// This signal handler is used to catch a signal caused when the user types
// CTRL-C to terminate the client. When this signal is catched, the client sends
// a LEAVE message to the server before exiting.
void
terminate(int signo)
{
    bzero(sendbuff, sizeof(sendbuff));
    strcpy(sendbuff, "LEAVE");
    if ((n = sendto(sockfd, sendbuff, sizeof(sendbuff), 0, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) {
        perror("send error");
        exit(0);
    }

    exit(0);
}

// retrieve the client's ip address and port number from the server message
struct client
get_client_info(char *mesg)
{
    char * token = strtok(mesg, " ");
    struct client cli;
    bzero(&cli, sizeof(cli));
    if (token != NULL) {
        token = strtok(NULL, " "); // the first token is either "JOIN" and "LEAVE" and therefore can be ignored
        strcpy(cli.ipaddr, token);
        token = strtok(NULL, " ");
        cli.port = atoi(token);
    }

    return cli;
}

// compare if the socket address of a client matches with a struct client
int
match(const struct client a, const struct sockaddr_in b)
{
    if ((strcmp(a.ipaddr, inet_ntoa(b.sin_addr)) == 0) && a.port == b.sin_port) {
        return 0;
    }

    return 1;
}

// compare the two clients to check if they're the same
int
compare(struct client a, struct client b) {
    int sameip = 0;
    int sameport = 0;
    if (strcmp(a.ipaddr, b.ipaddr) == 0) {
        sameip = 1;
    }

    if (a.port == b.port) {
        sameport = 1;
    }

    return sameip & sameport;
}

// parse and store the list of clients and their socket addresses that
// the server sends in response to a LEAVE message
void
populate_clients(char *mesg, struct client *clilist, int *max) {
    if (strlen(mesg) > 0) {
        // each client is delimited by a newline character
        int index = 0;
        char *token = "";
        while (token != NULL) {
            token = strtok(mesg, " ");
            if (token != NULL) {
                strcpy(clilist[index].ipaddr, token);
                token = strtok(NULL, "\n");
                clilist[index].port = atoi(token);
                (*max)++;
            }

            index++;
            mesg = NULL;
        }
    }
}

int
main(int argc, char **argv)
{
    int                maxfd, max, i, nready;
    socklen_t          clilen, servlen;
    struct sockaddr_in cliaddr;
    fd_set             rset, allset;
    char               recvbuff[MAXLINE];
    struct client      others[FD_SETSIZE];
    struct client      empty, tmpcli;

    if (argc != 3) {
        perror("usage: confclient <servhost> <servport>");
        exit(0);
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = atoi(argv[2]);

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        printf("inet_pton error for %s", argv[1]); // argv[1] == IP address
        exit(0);
    }

    // request to join the conference
    bzero(sendbuff, sizeof(sendbuff));
    strcpy(sendbuff, "JOIN");
    if ((n = sendto(sockfd, sendbuff, sizeof(sendbuff), 0, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) {
        perror("send error");
        exit(0);
    }

    // read the list of other clients from the server
    bzero(recvbuff, sizeof(recvbuff));
    if ((n = recvfrom(sockfd, recvbuff, MAXLINE, 0, NULL, NULL)) < 0) {
        perror("receive error");
        exit(0);
    }

    // set all the client entries to 0
    for (i = 0; i < FD_SETSIZE; i++) {
        bzero(&others[i], sizeof(others[i]));
    }
    max = -1;
    populate_clients(recvbuff, others, &max);

    FD_ZERO(&allset);
    FD_SET(fileno(stdin), &allset);
    FD_SET(sockfd, &allset);
    maxfd = max(sockfd, fileno(stdin));
    bzero(&empty, sizeof(empty));

    // set the signal handler when a client is terminated (when the user types CTRL-C to quit)
    signal(SIGINT, terminate);

    for ( ; ; ) {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        // socket is readable
        if (FD_ISSET(sockfd, &rset)) {
            bzero(recvbuff, sizeof(recvbuff));
            bzero(&cliaddr, sizeof(cliaddr));
            clilen = sizeof(cliaddr);
            if ((n = recvfrom(sockfd, recvbuff, MAXLINE, 0, (struct sockaddr *) &cliaddr, &clilen)) < 0) {
                perror("receive error");
                exit(0);
            }

            // a new client joins the conference
            if (strstr(recvbuff, "JOIN") != NULL) {
                tmpcli = get_client_info(recvbuff);
                // add a new client to the contact list
                for (i = 0; i < FD_SETSIZE; i++) {
                    if (compare(others[i], empty) == 1) {
                        others[i] = tmpcli;
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
            // a client exits the conference
            else if (strstr(recvbuff, "LEAVE") != NULL) {
                tmpcli = get_client_info(recvbuff);
                // remove the client from the contact list by zeroing out
                // the memory at the client's entry
                for (i = 0; i <= max; i++) {
                    if (compare(others[i], tmpcli) == 1) {
                        bzero(&others[i], sizeof(others[i]));
                    }
                }
            }
            // if a message is not a JOIN or LEAVE message, then it
            // should be a regular message coming from another client
            else {
                for (i = 0; i <= max; i++) {
                    if (compare(others[i], empty) != 1) {
                        if (match(others[i], cliaddr) == 0) {
                            break;
                        }
                    }
                }

                printf("Client %u: %s", i, recvbuff);
                fflush(stdout);
            }
        }

        // standard input is readable
        if (FD_ISSET(fileno(stdin), &rset)) {
            bzero(sendbuff, sizeof(sendbuff));
            if (fgets(sendbuff, MAXLINE, stdin) != NULL) {
                for (i = 0; i <= max; i++) {
                    if (compare(others[i], empty) != 1) {
                        bzero(&cliaddr, sizeof(cliaddr));
                        cliaddr.sin_family = AF_INET;
                        cliaddr.sin_port = others[i].port;
                        inet_pton(AF_INET, others[i].ipaddr, &cliaddr.sin_addr);
                        clilen = sizeof(cliaddr);
                        if ((n = sendto(sockfd, sendbuff, sizeof(sendbuff), 0, (struct sockaddr *) &cliaddr, clilen)) < 0) {
                            perror("send error");
                            exit(0);
                        }
                    }
                }
            }
            else { // when the user types CTRL-D to indicate EOF
                bzero(sendbuff, sizeof(sendbuff));
                strcpy(sendbuff, "LEAVE");
                if ((n = sendto(sockfd, sendbuff, sizeof(sendbuff), 0, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) {
                    perror("send error");
                    exit(0);
                }

                exit(0);
            }
        }
    }
}
