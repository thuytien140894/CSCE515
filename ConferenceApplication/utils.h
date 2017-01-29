//
// The header file for the conference client and server.
//
// Author: Tien Ho
// Date: 10/06/16.
//

#ifndef UTILS_H
#define UTILS_H

#include	<sys/socket.h>	/* basic socket definitions */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#define	MAXLINE	    4096	/* max text line length */
#define	BUFFSIZE    8192	/* buffer size for reads and writes */
#define LISTENQ       10

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

#endif //UTILS_H
