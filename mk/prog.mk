# $Id$

CFLAGS += -Wall -g -I${ROOT}/lib

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

${PROG}: ${OBJS}
	${CC} ${LIBS} -o ${.TARGET} ${OBJS}

clean:
	rm -rf ${PROG} ${OBJS} ${.CURDIR}/obj

obj:
	mkdir obj

depend:
	mkdep ${CFLAGS} ${OBJS:.o=.c:C/^/${.CURDIR}\//}
