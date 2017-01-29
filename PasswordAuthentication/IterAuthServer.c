//
// The client program to prompt the user for a username and password, which
// are then sent to the server for authentication.
//
// Author: Tien Ho
// Date:   9/20/16
//

#include "utils.h"

int
main(int argc, char **argv)
{
    int                listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t          len;
    char               buff[MAXLINE];
    char               recvusername[MAXCHAR];
    char               recvpasswrd[MAXCHAR];
    char               recvline[MAXLINE];
    userinfo           record[7];
    FILE *             userrecord;
    int                attempt = 0;
    int                success = 0;
    int                found = 0;

    // make sure that the port number and the password file are provided when the program
    // is executed
    if (argc != 3) {
        perror("usage: AuthClient <port> <password_file>");
        exit(0);
    }

    // open and read the password file
    userrecord = fopen(argv[2], "r"); // argv[2] = password file
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
        perror("socket error");
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
            for (i = 0; i < n; i++) {
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

        // closes the client socket after 3 failed attempts or a success
        close(connfd);
        success = 0;
        attempt = 0;
        found = 0;
    }
}
