# $Id$

SUBDIRS += common pcred

all clean depend obj:
.for i in ${SUBDIRS}
	@echo "===> ${.CURDIR}/$i"
	@(cd $i && make ${.TARGET})
	@echo "<=== ${.CURDIR}"
.endfor
