# $Id$

TARGET = pcred
OBJS = main.o

all: ${TARGET}

$(TARGET): ${OBJS}
	${CC} ${LIBS} -o ${.TARGET} ${OBJS}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC}

clean:
	rm -f ${TARGET} ${OBJS}
