# $Id$

OBJS = lbuf.o
TARGET = ${OBJS}

all: ${TARGET}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

clean:
	rm -rf ${OBJS} obj

depend:
	mkdep ${OBJS:.o=.c}

obj:
	mkdir obj
