# $Id$

TARGET = pcred
OBJS = pcred.o ../common/lbuf.o
CFLAGS = -Wall -I../common

all: ${TARGET}

$(TARGET): ${OBJS}
	${CC} ${LIBS} -o ${.TARGET} ${OBJS}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

clean:
	rm -f ${TARGET} ${OBJS}
