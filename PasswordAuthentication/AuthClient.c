//
// The iterative server program to authenticate a username and password pair
// received from a client, using a provided file of legitimate pairs of usernames
// and passwords. This server is iterative because it can only serve one client at
// a time.
//
// Author: Tien Ho
// Date:   9/20/16
//

#include "utils.h"

int
main(int argc, char **argv)
{
    int                sockfd;
    struct sockaddr_in servaddr;
    char               username[MAXCHAR];
    char               password[MAXCHAR];
    char               buff[MAXLINE];
    char               recvmsg[MAXLINE];

    // ensure that the IP address and the port number are provided when the program is executed
    if (argc != 3) {
        perror("usage: AuthClient <IPaddress> <port>");
        exit(0);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket error");
        exit(0);
    }

    // specify the ip address and port number of the server to connect
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2])); // argv[2] = port

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        printf("inet_pton error for %s", argv[1]); // argv[1] == IP address
        exit(0);
    }

    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("connect error");
        exit(0);
    }

    printf("Welcome!\n");

    // only ask the user to reenter the username and password if
    // he/she has not succeeded and has not passed the limited number of
    // authentication attempts
    strcpy(recvmsg, "failure");
    while (strcmp(recvmsg, "failure") == 0) {
        // read in the input username and password
        printf("Enter your username: ");
        bzero(username, sizeof(username));
        scanf("%s", username);
        printf("Enter your password: ");
        bzero(password, sizeof(password));
        scanf("%s", password);
        printf("\n");

        // combine the username and password into one
        // single message to send to the server
        bzero(buff, sizeof(buff));
        strcpy(buff, username);
        strcat(buff, " ");
        strcat(buff, password);

        // send the username and password to the server
        if (write(sockfd, buff, strlen(buff)) < 0) {
            perror("write error");
            exit(0);
        }

        if (read(sockfd, recvmsg, MAXLINE) < 0) {
            perror("read error");
            exit(0);
        }
    }

    if (strcmp(recvmsg, "success") == 0) {
        printf("Logged in successfully\n");
    }
    else if (strcmp(recvmsg, "final failure") == 0) {
        printf("Failed to authenticate\n");
    }
    else {
        perror("cannot read server feedback");
        exit(0);
    }

    // exit when the connection is closed by the server
    if (read(sockfd, recvmsg, strlen(recvmsg)) == 0) {
        exit(0);
    }
}
