# $Id$

SUBDIRS += pcred
TARGET = putils

all: ${TARGET}

putils clean depend obj:
.for i in ${SUBDIRS}
	(cd $i && make ${.TARGET})
.endfor
