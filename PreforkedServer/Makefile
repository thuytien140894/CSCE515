PROGS =	 echoserver echoclient

CC = gcc
CFLAGS = -g
CLEANFILES = core core.* *.core *.o


all:	${PROGS}

echoserver:	echoserver.o
		${CC} ${CFLAGS} -o $@ echoserver.o

echoclient:	echoclient.o
		${CC} ${CFLAGS} -o $@ echoclient.o

clean:
		rm -f ${PROGS} ${CLEANFILES}
