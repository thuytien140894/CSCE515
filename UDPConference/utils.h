//
// The header file for the conference client and server.
//
// Author: Tien Ho
// Date: 11/01/16.
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

#define	MAXLINE	    4096	/* max text line length */
#define MAXCHAR       30
#define	BUFFSIZE    8192	/* buffer size for reads and writes */
#define LISTENQ       10

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

struct client {
    char ipaddr[MAXCHAR];
    int  port;
};

#endif //UTILS_H
