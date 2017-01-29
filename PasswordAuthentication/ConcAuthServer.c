//
// The concurrent server program to authenticate a username and password pair
// received from a client, using a provided file of legitimate pairs of usernames
// and passwords. This server is concurrent because it can serve multiple clients at
// the same time by dispatching a child process for each client.
//
// Author: Tien Ho
// Date:   9/20/16
//

#include "utils.h"

int
authenticate(const int connfd, const userinfo *record)
{
    char               recvusername[MAXCHAR];
    char               recvpasswrd[MAXCHAR];
    char               buff[MAXLINE];
    char               recvline[MAXLINE];
    int                attempt = 0;
    int                success = 0;
    int                found = 0;

    // only perform authentication if the client has not succeeded
    // and has not exceeded the allowed number of authentication attempts
    while (attempt != 3 && success != 1) {
        bzero(recvline, sizeof(recvline));
        if (read(connfd, recvline, MAXLINE) < 0) {
            perror("read error");
            exit(0);
        }

        // the username and password pair sent from the client
        // is assumed to be separated by a space
        char * token = strtok(recvline, " ");
        if (token != NULL) {
            bzero(recvusername, sizeof(recvusername));
            strcpy(recvusername, token);
            token = strtok(NULL, " "); // scan from the end of the last token
            bzero(recvpasswrd, sizeof(recvpasswrd));
            strcpy(recvpasswrd, token);
        }

        // check the record to find any matching for the client's provided username and password
        size_t n = sizeof(record) / sizeof(record[0]);
        size_t i;
        for (i = 0; i < sizeof(record); i++) {
            if (strcmp(record[i].username, recvusername) == 0) {
                found = 1;
                if (strcmp(record[i].password, recvpasswrd) == 0) {
                    strcpy(buff, "success");
                    success = 1;
                    break;
                }
                else {
                    strcpy(buff, "failure");
                    attempt++;
                    break;
                }
            }
        }

        if (found == 0) {
            strcpy(buff, "failure");
            attempt++;
        }

        // send the final failure message to the client
        // to indicate that the server no longer allows any
        // additional authentication attempts
        if (attempt == 3) {
            strcpy(buff, "final failure");
        }

        if (write(connfd, buff, strlen(buff)) < 0) {
            perror("write error");
            exit(0);
        }
    }

    return 0;
}

int
main(int argc, char **argv)
{
    pid_t              pid;
    int                listenfd, connfd, n;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t          len;
    userinfo           record[7];
    FILE *             userrecord;
    char               buff[MAXLINE];

    if (argc != 3) {
        perror("usage: AuthClient <port> <password>");
        exit(0);
    }

    // open and read the password file
    userrecord = fopen(argv[2], "r");
    int index = 0;
    while (fgets(buff, MAXCHAR, userrecord) != NULL) {
        // each username and password pair is separated by a space
        char * token = strtok(buff, " ");
        if (token != NULL) {
            bzero(record[index].username, sizeof(record[index].username));
            strcpy(record[index].username, token);
            token = strtok(NULL, " "); // scan from the end of the last token
            bzero(record[index].password, sizeof(record[index].password));
            strncpy(record[index].password, token, strlen(token)-1);
        }
        index++;
    }

    // create a listen socket and bind it to the server's wellknown address
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("listen error");
        exit(0);
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1])); // argv[1] = port

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("error in binding");
        exit(0);
    }

    if (listen(listenfd, LISTENQ) < 0)
        exit(0);

    for ( ; ; ) {
        connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len);
        if (connfd < 0) {
            perror("connection error");
            continue;
        }

        // dispatch a child server process for each established client
        pid = fork();
        if (pid == 0) {
            close(listenfd);
            n = authenticate(connfd, record);
            close(connfd);
            exit(0);
        }

        // close the current connection to welcome more clients
        // and process them concurrently
        close(connfd);
    }
}
