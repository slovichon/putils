# $Id$

CFLAGS += -Wall -I${SYSROOT}/lib/c

all: ${TARGET}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

$(TARGET): ${OBJS}
	${CC} ${LIBS} -o ${.TARGET} ${OBJS}

clean:
	rm -rf ${TARGET} ${OBJS} ${.CURDIR}/obj

obj:
	mkdir obj

depend:
	mkdep ${CFLAGS} ${OBJS:.o=.c:C/^/${.CURDIR}\//}
