# $Id$

all: ${LIB}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

${LIB}: ${OBJS}
	${AR} cq ${LIB} `${LORDER} ${OBJS} | tsort -q`
	${RANLIB} ${LIB}

clean:
	rm -rf ${LIB} ${OBJS} ${TEST} ${TEST}.o

obj:
	mkdir obj

depend:
	mkdep ${CFLAGS} ${OBJS:.o=.c:C/^/${.CURDIR}\//}

test: ${TEST}.o ${OBJS}
	${CC} ${LIBS} -o ${TEST} ${OBJS} ${TEST}.o
