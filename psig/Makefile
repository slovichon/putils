# $Id$

SYSROOT = ..

.include "../mk/defs.mk"

PROG = psig
OBJS = psig.o ${LIBDIR}/libputils.a
LIBS = -lkvm
MAN = psig.1
BINMODE = 2555
BINGRP = kmem
TEST = handler

.include "../mk/prog.mk"
