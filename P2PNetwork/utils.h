//
// The header file for the peer program.
//
// Author: Tien Ho
// Date: 11/23/16.
//

#ifndef UTILS_H
#define UTILS_H

#include	<sys/socket.h>	/* basic socket definitions */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include    <signal.h>
#include    <fcntl.h>
#include    <netdb.h>

#define	MAXLINE	    4096	/* max text line length */
#define MAXCHAR       30
#define	BUFFSIZE    8192	/* buffer size for reads and writes */
#define LISTENQ       10
#define YES            1
#define NO             2
#define CONNECTING     3    /* connect() in progress */
#define ESTABLISHED    4    /* connect() complete; now reading */
#define INACTIVE       5
#define DONE           6

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

struct peer {
    char ipaddr[MAXCHAR];
    int  port;
};

struct peerconn {
    char ipaddr[MAXCHAR];
    int  port;
    int  hostport;
    char hostipaddr[MAXCHAR];
    int  flag;
    int  seqnum;
    int  fd;
};

#endif //UTILS_H
