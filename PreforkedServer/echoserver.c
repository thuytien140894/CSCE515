//
// This program simulates an echo server that echoes a message back to its
// client. The program implements a TCP preforked server with file locking
// around accept. Accordingly, the server first creates a pool of child
// processes, each handling each client request. In order to avoid the
// thundering herd, a lock is used so that only one child is blocked in the
// call to accept at a time. The program accepts two arguments, the port
// number and the number of children to create.
//
// Author: Tien Ho
// Date:   12/01/16
//

#include "utils.h"

// global variables
static int          nchildren;
static pid_t        *pids;
static struct flock lock_it, unlock_it;
static int          lock_fd = -1;

void
lock_init(char *pathname)
{
    char lock_file[1024];
    mode_t FILE_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    strncpy(lock_file, pathname, sizeof(lock_file));
    mkstemp(lock_file);
    lock_fd = open(lock_file, O_CREAT | O_WRONLY, FILE_MODE);
    // remove the pathname from the directory but lock_fd remains open
    unlink(lock_file);

    lock_it.l_type = F_WRLCK;
    lock_it.l_whence = SEEK_SET;
    lock_it.l_start = 0;
    lock_it.l_len = 0;

    unlock_it.l_type = F_UNLCK;
    unlock_it.l_whence = SEEK_SET;
    unlock_it.l_start = 0;
    unlock_it.l_len = 0;
}

// Obtain the lock for the lock_file
void
lock_wait()
{
    int rc;

    while ((rc = fcntl(lock_fd, F_SETLKW, &lock_it)) < 0) {
        if (errno = EINTR)
            continue; // the lock is already obtained by some process
        else
            perror("fcntl error for lock_wait()");
    }
}

// Release the lock on the lock_file for other processes to use
void
lock_release()
{
    if (fcntl(lock_fd, F_SETLKW, &unlock_it) < 0)
        perror("fcntl error for lock_release");
}

pid_t
fork_child(int i, int listenfd, int addrlen)
{
    pid_t pid;
    void  child_main(int, int, int);

    if ((pid = fork()) > 0)
        return pid;

    child_main(i, listenfd, addrlen);
}

void
child_main(int i, int listenfd, int addrlen)
{
    int                connfd, n;
    struct sockaddr_in cliaddr;
    socklen_t          clilen;
    char               buff[MAXLINE];

    printf("child %ld starting\n", (long) getpid());
    for ( ; ; ) {
        lock_wait();
        connfd = accept(listenfd, NULL, NULL);
        lock_release();

        clilen = addrlen;
        if (getpeername(connfd, (struct sockaddr *) &cliaddr, &clilen) < 0)
            perror("peer name error");

        printf("Connected to client on \'%s\' at port \'%d\'\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
        fflush(stdout);

        for ( ; ; ) {
            bzero(buff, sizeof(buff));
            if ((n = read(connfd, buff, MAXLINE)) == 0) { // The client exits
                printf("Disconnected from client on \'%s\' at port \'%d\'\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
                break;
            }
            else if (n > 0) { // echo the message back to the client
                if (write(connfd, buff, strlen(buff)) < 0)
                    perror("write error");
            }
            else
                perror("read error");
        }

        close(connfd);
    }
}

void
sig_int(int signo)
{
    int i;

    for (i = 0; i < nchildren; i++)
        kill(pids[i], SIGTERM);
    while (wait(NULL) > 0)
        ;

    if (errno != ECHILD)
        perror("wait error");

    exit(0);
}

int
main(int argc, char **argv)
{
    int                listenfd, i;
    socklen_t          addrlen;
    struct sockaddr_in servaddr;

    if (argc != 3) {
        perror("usage: echoserver <port> <children>");
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
    servaddr.sin_port = atoi(argv[1]);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("error in binding");
        exit(0);
    }

    if (listen(listenfd, LISTENQ) < 0)
        exit(0);

    nchildren = atoi(argv[2]);
    pids = calloc(nchildren, sizeof(pid_t));

    // create a lock file for all the children processes
    lock_init("/tmp/lock.XXXXXX");
    for (i = 0; i < nchildren; i++)
        pids[i] = fork_child(i, listenfd, addrlen);

    // when a user presses CTRL-C
    signal(SIGINT, sig_int);

    // the rest of the program is processed by children and the parent can sleep
    for ( ; ; )
        pause();
}
