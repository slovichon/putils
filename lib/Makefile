# $Id$

OBJS = lbuf.o
TARGET = ${OBJS}
CFLAGS = -Wall

all: ${TARGET}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

clean:
	rm -rf ${OBJS} ${.CURDIR}/obj

depend:
	mkdep ${OBJS:.o=.c}

obj:
	mkdir obj
