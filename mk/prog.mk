# $Id$

CFLAGS += -Wall -I../lib/c

all: ${TARGET}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

$(TARGET): ${OBJS}
	${CC} ${LIBS} -o ${.TARGET} ${OBJS}

clean:
	rm -f ${TARGET} ${OBJS}

obj:
	mkdir obj

depend:
	mkdep ${OBJS:.o=.c}
