# $Id$

CFLAGS += -Wall -g -I${ROOT}/lib

all: ${LIB}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

${LIB}: ${OBJS}
	${AR} cq ${LIB} `${LORDER} ${OBJS} | tsort -q`
	${RANLIB} ${LIB}

clean:
	rm -rf ${LIB} ${OBJS}

obj:
	mkdir obj

depend:
	mkdep ${CFLAGS} ${OBJS:.o=.c:C/^/${.CURDIR}\//}
