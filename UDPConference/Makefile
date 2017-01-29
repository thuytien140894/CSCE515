PROGS =	 confserver confclient

CC = gcc
CFLAGS = -g
CLEANFILES = core core.* *.core *.o


all:	${PROGS}

confserver:	confserver.o
		${CC} ${CFLAGS} -o $@ confserver.o

confclient:	confclient.o
		${CC} ${CFLAGS} -o $@ confclient.o

clean:
		rm -f ${PROGS} ${CLEANFILES}
