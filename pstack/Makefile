# $Id$

SYSROOT = ..

.include "../mk/defs.mk"

PROG = pstack
OBJS = md.o pstack.o ${LIBDIR}/libputils.a
LIBS = -lkvm
BINMODE = 2555
BINGRP = kmem

.include "../mk/prog.mk"
