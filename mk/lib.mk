# $Id$

ROOT = ${.CURDIR}/${SYSROOT}

CFLAGS += -Wall -g -I${ROOT}/lib

all: ${LIB}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

${LIB}: ${OBJS}
	${AR} cq ${LIB} `${LORDER} ${OBJS} | tsort -q`
	${RANLIB} ${LIB}

clean:
	rm -rf ${LIB} ${OBJS} ${.CURDIR}/obj

obj:
	mkdir obj

depend:
	mkdep ${CFLAGS} ${OBJS:.o=.c:C/^/${.CURDIR}\//}
