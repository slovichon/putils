# $Id$

all: ${PROG}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

${PROG}: ${OBJS}
	${CC} ${LIBS} -o ${.TARGET} ${OBJS}

clean:
	rm -rf ${PROG} ${OBJS}

obj:
	mkdir obj

depend:
	mkdep ${CFLAGS} ${OBJS:M*.o:.o=.c:C/^/${.CURDIR}\//}
