# $Id$

TARGET = pcred
OBJS = pcred.o ../lib/lbuf.o
CFLAGS = -Wall -I../lib

all: ${TARGET}

$(TARGET): ${OBJS}
	${CC} ${LIBS} -o ${.TARGET} ${OBJS}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

clean:
	rm -f ${TARGET} ${OBJS}
