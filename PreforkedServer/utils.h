//
// The header file for the echo client and server.
//
// Author: Tien Ho
// Date: 12/01/16.
//

#ifndef UTILS_H
#define UTILS_H

#include	<sys/socket.h>	/* basic socket definitions */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include    <fcntl.h>
#include    <unistd.h>
#include    <netdb.h>
#include    <signal.h>

#define	MAXLINE	    4096	/* max text line length */
#define	BUFFSIZE    8192	/* buffer size for reads and writes */
#define LISTENQ       10

#endif //UTILS_H
