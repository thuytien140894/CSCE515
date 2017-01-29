PROGS =	 AuthClient IterAuthServer ConcAuthServer

CC = gcc
CFLAGS = -g 
CLEANFILES = core core.* *.core *.o 


all:	${PROGS}

AuthClient:	AuthClient.o
		${CC} ${CFLAGS} -o $@ AuthClient.o

IterAuthServer:	IterAuthServer.o
		${CC} ${CFLAGS} -o $@ IterAuthServer.o

ConcAuthServer:	ConcAuthServer.o
		${CC} ${CFLAGS} -o $@ ConcAuthServer.o

clean:
		rm -f ${PROGS} ${CLEANFILES}
