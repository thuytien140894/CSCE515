//
// This program simulates an echo client that sends the user input to an
// echo server and then receives back the same message to display on the
// screen. The client uses a child to wait for the user input and send it to
// the server. The parent process waits for the messages from the server
// and display them to the standard output.
//
// Author: Tien Ho
// Date:   12/01/16
//

#include "utils.h"

void
sig_chld(int signo)
{
    pid_t pid;
    int   stat;

    pid = wait(&stat);
    exit(0);
}

void
send_message(FILE *fp, int sockfd)
{
    char buff[MAXLINE];

    while (fgets(buff, MAXLINE, fp) != NULL) {
        if (write(sockfd, buff, strlen(buff)) < 0)
            perror("write error");
    }
}

int
main(int argc, char **argv)
{
    int                sockfd, i, n;
    pid_t              pid;
    struct sockaddr_in servaddr;
    char               recvbuff[MAXLINE], ipaddr[MAXLINE];
    struct in_addr     **addr_list;
    struct hostent     *hp;

    if (argc != 3) {
        perror("usage: echoclient <servhost> <servport>");
        exit(0);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = atoi(argv[2]);

    // get the ip address from the hostname
    hp = gethostbyname(argv[1]);
    addr_list = (struct in_addr **)hp->h_addr_list;
    strcpy(ipaddr, "");
    for(i = 0; addr_list[i] != NULL; i++) {
        strcat(ipaddr, inet_ntoa(*addr_list[i]));
    }

    if (inet_pton(AF_INET, ipaddr, &servaddr.sin_addr) <= 0) {
        printf("inet_pton error for %s", argv[1]); // argv[1] == localhost
        exit(0);
    }

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(0);
    }

    printf("Connected to server on \'%s\' at port \'%s\'\n", argv[1], argv[2]);

    // signal the server when a child terminates
    signal(SIGCHLD, sig_chld);

    // a child is forked to wait for the user input and send it to the server
    pid = fork();
    if (pid == 0) {
        send_message(stdin, sockfd);
        close(sockfd);
        exit(0);
    }

    // the parent waits for the server's messages
    for ( ; ; ) {
        bzero(recvbuff, sizeof(recvbuff));
        if ((n = read(sockfd, recvbuff, MAXLINE)) == 0) { // the server exits
            printf("Disconnected from server\n");
            close(sockfd);
            exit(0);
        }
        else if (n > 0) {
            printf(">> %s", recvbuff);
            fflush(stdout);
        }
        else
            perror("read error");
    }
}
