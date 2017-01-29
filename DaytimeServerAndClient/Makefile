
PROGS =	 daytimetcpcli daytimetcpsrv

CC = gcc
CFLAGS = -g 
CLEANFILES = core core.* *.core *.o 


all:	${PROGS}

daytimetcpcli:	daytimetcpcli.o
		${CC} ${CFLAGS}  -o $@ daytimetcpcli.o 

daytimetcpsrv:	daytimetcpsrv.o
		${CC} ${CFLAGS} -o $@ daytimetcpsrv.o 


clean:
		rm -f ${PROGS} ${CLEANFILES}
