//
// This program stimulates a peer in a P2P network. This peer accepts three
// arguments: the port number, the maximum number of peers to connect, and the
// peersfile that contains the hostname and ip for all potential peers.
// The peer acts as both a server and a client. It creates a listening socket
// to accept incoming requests as well as tries to connect to as many peers as
// it is allowed. The peer uses nonblocking connect to try to connect to
// other peers who might or might not be running at that instant. In this way,
// the peer does not need to wait for the connection to be completely
// successful before moving on. Any user's input is sent to the peer's direct
// and nondirect connections. In order to avoid duplication, each message is
// encoded with the sender's ip, port, and sequence number.
//
// Author: Tien Ho
// Date:   11/23/16
//

#include "utils.h"

// global variables
int                maxfd, npeers, max, n, i;
fd_set             rset, wset, rs, ws;
char               buff[MAXLINE];
struct sockaddr_in peeraddr, localaddr, servaddr;
socklen_t          addrlen;
FILE               *peersfile;
struct peerconn    currentpeers[FD_SETSIZE];
struct peerconn    empty;

int
count_lines(char *filename)
{
    int line = 0;
    FILE *file = fopen(filename, "r");
    while (fgets(buff, MAXLINE, file)) {
        line++;
    }
    fclose(file);

    return line;
}

int
compare_peer(const struct peerconn *a, char *ipaddr, int port)
{
    int sameip = 0;
    int sameport = 0;

    if (strcmp(a->ipaddr, ipaddr) == 0)
        sameip = 1;

    if (a->port == port)
        sameport = 1;

    return sameip & sameport;
}

int
compare_host(const struct peerconn *a, char *ipaddr, int port)
{
    int sameip = 0;
    int sameport = 0;

    if (strcmp(a->hostipaddr, ipaddr) == 0)
        sameip = 1;

    if (a->hostport == port)
        sameport = 1;

    return sameip & sameport;
}

int
is_self(char *ip, int port)
{
    int sameip = 0;
    int sameport = 0;
    char *localhost = "127.0.0.1"; // assume this program is run on the local machine

    if (strcmp(localhost, ip) == 0)
        sameip = 1;

    if (ntohs(servaddr.sin_port) == port)
        sameport = 1;

    return sameip & sameport;
}

struct peer *
read_peers(char *filename)
{
    int            i;
    struct in_addr **addr_list;
    struct hostent *hp;

    // open the peersfile and store the info
    bzero(buff, sizeof(buff));
    peersfile = fopen(filename, "r");
    npeers = count_lines(filename);
    static struct peer *allpeers;
    allpeers = malloc(npeers * sizeof(struct peer));

    for (i = 0; i < npeers; i++) {
        bzero(&allpeers[i], sizeof(struct peer));
    }

    int index = 0;
    while (fgets(buff, MAXLINE, peersfile) != NULL) {
        char *token = strtok(buff, " ");
        if (token != NULL) {
            // get the ip address from the hostname
            char ip[MAXLINE];
            bzero(allpeers[index].ipaddr, sizeof(allpeers[index].ipaddr));
            hp = gethostbyname(token);
            addr_list = (struct in_addr **)hp->h_addr_list;
            strcpy(ip, "");
            for(i = 0; addr_list[i] != NULL; i++) {
                strcat(ip, inet_ntoa(*addr_list[i]));
            }
            token = strtok(NULL, " ");
            int port = atoi(token);
            if (is_self(ip, port) == 1) { // only add peers that is not itself
                continue;
            }
            strcpy(allpeers[index].ipaddr, ip);
            allpeers[index].port = atoi(token);
        }
        index++;
    }
    fclose(peersfile);

    return allpeers;
}

int
peer_connect(struct peer *newpeer)
{
    int sockfd, flags;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(0);
    }

    // set the socket nonblocking
    flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    bzero(&peeraddr, sizeof(peeraddr));
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_port = htons(newpeer->port);

    if (inet_pton(AF_INET, newpeer->ipaddr, &peeraddr.sin_addr) <= 0) {
        printf("inet_pton error for %s", newpeer->ipaddr);
        exit(0);
    }

    // initiate nonblocking connect to the peer
    if (connect(sockfd, (struct sockaddr *) &peeraddr, sizeof(peeraddr)) < 0) {
        if (errno != EINPROGRESS) {
            perror("nonblocking connect error");
        }

        // get local address
        bzero(&localaddr, sizeof(localaddr));
        addrlen = sizeof(localaddr);
        if (getsockname(sockfd, (struct sockaddr *) &localaddr, &addrlen) < 0)
            perror("socket name error");

        // remember this new peer connection
        for (i = 0; i < FD_SETSIZE; i++) {
            if (n = memcmp(&currentpeers[i], &empty, sizeof(empty)) == 0) {
                break;
            }
        }
        currentpeers[i].hostport = localaddr.sin_port;
        strcpy(currentpeers[i].hostipaddr, inet_ntoa(localaddr.sin_addr));
        currentpeers[i].flag = CONNECTING;
        currentpeers[i].fd = sockfd;
        if (max < i)
            max = i;

        FD_SET(sockfd, &rset);
        FD_SET(sockfd, &wset);
        if (sockfd > maxfd)
            maxfd = sockfd;
    }
}

int
main(int argc, char **argv)
{
    int                listenfd, connfd, maxpeers, nconn, flag, fd, error, seqnum, received, peerfd, sender, found;
    struct sockaddr_in cliaddr;
    char               sendbuff[MAXLINE], recvbuff[MAXLINE];

    if (argc != 4) {
        perror("usage: peer <port> <maxpeers> <peersfile>");
        exit(0);
    }

    // create a listen socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("error in binding");
        exit(0);
    }

    if (listen(listenfd, LISTENQ) < 0)
        exit(0);

    struct peer *allpeers = read_peers(argv[3]);

    FD_ZERO(&rset);
    FD_ZERO(&wset);
    FD_SET(fileno(stdin), &rset); // standard input
    FD_SET(listenfd, &rset);
    maxfd = max(fileno(stdin), listenfd);
    maxpeers = atoi(argv[2]);
    if (maxpeers > npeers)
        maxpeers = npeers;

    bzero(&empty, sizeof(empty));
    for (i = 0; i < FD_SETSIZE; i++) {
        bzero(&currentpeers[i], sizeof(struct peerconn));
    }

    // establish connections with other peers
    int index = 0;
    nconn = 0;
    while (nconn < maxpeers) {
        peer_connect(&allpeers[index]);
        nconn++;
        index++;
    }

    for ( ; ; ) {
        rs = rset;
        ws = wset;
        n = select(maxfd + 1, &rs, &ws, NULL, NULL);
        // a new connection arrives
        if (FD_ISSET(listenfd, &rs)) {
            if ((connfd = accept(listenfd, NULL, NULL)) < 0) {
                perror("connection error");
                continue;
            }

            // get local address
            bzero(&localaddr, sizeof(localaddr));
            addrlen = sizeof(localaddr);
            if (getsockname(connfd, (struct sockaddr *) &localaddr, &addrlen) < 0)
                perror("socket name error");

            // get peer address
            bzero(&cliaddr, sizeof(cliaddr));
            addrlen = sizeof(cliaddr);
            if (getpeername(connfd, (struct sockaddr *) &cliaddr, &addrlen) < 0)
                perror("peer name error");

            for (i = 0; i < FD_SETSIZE; i++) {
                if ((n = memcmp(&currentpeers[i], &empty, sizeof(empty))) == 0) {
                    break;
                }
            }
            // remember this new peer
            strcpy(currentpeers[i].ipaddr, inet_ntoa(cliaddr.sin_addr));
            currentpeers[i].port = cliaddr.sin_port;
            currentpeers[i].hostport = localaddr.sin_port;
            strcpy(currentpeers[i].hostipaddr, inet_ntoa(localaddr.sin_addr));
            currentpeers[i].fd = connfd;
            currentpeers[i].flag = ESTABLISHED;
            currentpeers[i].seqnum = 0;
            if (max < i)
                max = i;

            // add this connection for to the readset for future select
            FD_SET(connfd, &rset);
            if (connfd > maxfd) {
                maxfd = connfd;
            }
            nconn++;
            printf("connection established for \"%s %d\"\n", currentpeers[i].ipaddr, currentpeers[i].port);
        }

        for (i = 0; i <= max; i++) {
            flag = currentpeers[i].flag;
            if (flag == 0 || flag == INACTIVE)
                continue;
            fd = currentpeers[i].fd;
            // check for nonblocking connection
            if (flag == CONNECTING && (FD_ISSET(fd, &rs) || FD_ISSET(fd, &ws))) {
                n = sizeof(error);
                // address both Berkeley-deprived implementations and Solaris
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &n) < 0 || error != 0) {
                    currentpeers[i].flag = INACTIVE;
                    perror("nonblocking connect failed");
                } else {
                    FD_CLR(fd, &wset);
                    // get peer address
                    bzero(&cliaddr, sizeof(cliaddr));
                    addrlen = sizeof(cliaddr);
                    if (getpeername(fd, (struct sockaddr *) &cliaddr, &addrlen) < 0)
                        perror("peer name error");

                    // remember the new peer
                    strcpy(currentpeers[i].ipaddr, inet_ntoa(cliaddr.sin_addr));
                    currentpeers[i].port = cliaddr.sin_port;
                    currentpeers[i].flag = ESTABLISHED;
                    currentpeers[i].seqnum = 0;

                    printf("connection established for \"%s %d\"\n", currentpeers[i].ipaddr, currentpeers[i].port);
                }
            }
            // one of the existing connection becomes readable
            else if (flag == ESTABLISHED && FD_ISSET(fd, &rs)) {
                bzero(recvbuff, sizeof(recvbuff));
                if ((n = read(fd, recvbuff, sizeof(recvbuff))) == 0) { // the peer quits
                    close(fd);
                    FD_CLR(fd, &rset);
                    currentpeers[i].flag = DONE;
                    nconn--;

                    printf("disconnection from \"%s %d\"\n", currentpeers[i].ipaddr, currentpeers[i].port);
                }
                else if (n > 0) {
                    // parse the message for ip, port, and sequence number
                    bzero(buff, sizeof(buff));
                    strcpy(buff, recvbuff);
                    char *token = strtok(buff, ":");
                    char ipaddr[MAXLINE], message[MAXLINE];
                    int  port, seq;
                    if (token != NULL) {
                        strcpy(ipaddr, token);
                        token = strtok(NULL, ":");
                        port = atoi(token);
                        token = strtok(NULL, ":");
                        seq = atoi(token);
                        token = strtok(NULL, ":");
                        strcpy(message, token);
                    }

                    // first check if the current host sent the received message itself
                    // for example, two peers might connect to one another.
                    // Therefore, a message sent from one peer will go back to itself.
                    sender = NO;
                    for (i = 0; i <= max; i++) {
                        if (compare_host(&currentpeers[i], ipaddr, port) == 1) {
                            sender = YES;
                        }
                    }

                    // check if the current host received this message before
                    // using the sequence number of the message
                    if (sender == NO) {
                        received = YES;
                        found = NO;
                        struct peerconn peer;
                        for (i = 0; i <= max; i++) {
                            peer = currentpeers[i];
                            if (currentpeers[i].flag == ESTABLISHED) {
                                if (compare_peer(&peer, ipaddr, port) == 1) {
                                    found = YES;
                                    if (peer.seqnum != seq) { // if the message has not been received before
                                        received = NO;
                                        break;
                                    }
                                }
                            }
                        }

                        // found is used in case when a message is sent from a peer
                        // that is not directly connected to the current host
                        if (received == NO || found == NO) {
                            if (received == NO) {
                                currentpeers[i].seqnum = seq; // update the sequence number
                            }
                            printf("Peer %s %d: %s", ipaddr, port, message);
                            fflush(stdout);

                            // relay the message to all the connected peers
                            for (i = 0; i <= max; i++) {
                                if (currentpeers[i].flag == ESTABLISHED) {
                                    peerfd = currentpeers[i].fd;
                                    if (peerfd != fd) { // only send to peers who are not the senders of the message
                                        if (write(peerfd, recvbuff, strlen(recvbuff)) < 0) {
                                            perror("write error");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else {
                    perror("read error");
                }
            }
        }
        // standard input is readable
        if (FD_ISSET(fileno(stdin), &rs)) {
            if (nconn > 0) {
                bzero(buff, sizeof(buff));
                bzero(sendbuff, sizeof(sendbuff));
                if (fgets(buff, MAXLINE, stdin) != NULL) {
                    seqnum++; // increment the sequence number for messages send from the current host
                    for (i = 0; i <= max; i++) {
                        flag = currentpeers[i].flag;
                        // send the message to all the connected peers
                        if (flag == ESTABLISHED) {
                            peerfd = currentpeers[i].fd;
                            // send the ip, port number, and sequence nummber along with the message
                            // ip:port:seqnum serves as the id for the message used for duplication detection
                            sprintf(sendbuff, "%s:%d:%d:%s", currentpeers[i].hostipaddr, currentpeers[i].hostport,
                                    seqnum, buff);
                            if (write(peerfd, sendbuff, strlen(sendbuff)) < 0) {
                                perror("write error");
                                exit(0);
                            }
                        }
                    }
                }
            }
        }
    }
}
