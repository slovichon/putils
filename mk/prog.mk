# $Id$

CFLAGS += -Wall -g -I${SYSROOT}/lib

.ifdef TARGET
all: ${TARGET}
.else
all: ${PROG}
.endif

.c.o:
	@if [ X"${.TARGET:M*/*}" ]; then		\
		echo "${CC} ${CFLAGS} -c ${.IMPSRC}";	\
		${CC} ${CFLAGS} -c ${.IMPSRC};		\
	else						\
		(cd && make ${.TARGET});		\
	fi

${PROG}: ${OBJS}
	${CC} ${LIBS} -o ${.TARGET} ${OBJS}

clean:
	rm -rf ${PROG} ${OBJS} ${.CURDIR}/obj

obj:
	mkdir obj

depend:
	mkdep ${CFLAGS} ${OBJS:.o=.c:C/^/${.CURDIR}\//}
