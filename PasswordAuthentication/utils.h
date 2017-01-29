//
// The header file for the authentication client and servers.
//
// Author: Tien Ho
// Date: 9/20/16.
//

#ifndef PASSWORDAUTHENTICATION_UTILS_H
#define PASSWORDAUTHENTICATION_UTILS_H

#include	<sys/socket.h>	/* basic socket definitions */
#include	<arpa/inet.h>	/* inet(3) functions */
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#define	MAXLINE	    4096	/* max text line length */
#define MAXCHAR       30
#define	MAXSOCKADDR  128	/* max socket address structure size */
#define	BUFFSIZE    8192	/* buffer size for reads and writes */
#define LISTENQ       10

struct userinfo {
    char username[MAXCHAR];
    char password[MAXCHAR];
};

typedef struct userinfo userinfo;

#endif //PASSWORDAUTHENTICATION_UTILS_H
