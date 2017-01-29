PROGS =	 peer

CC = gcc
CFLAGS = -g
CLEANFILES = core core.* *.core *.o


all:	${PROGS}

confserver:	peer.o
		${CC} ${CFLAGS} -o $@ peer.o

clean:
		rm -f ${PROGS} ${CLEANFILES}
