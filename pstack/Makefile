# $Id$

PROG=		pstack
OBJS=		pstack.c ${MACHINE_ARCH}.c
LDADD+=		-lkvm
DPADD+=		${LIBKVM}
BINMODE=	2555
BINGRP=		kmem

.include <bsd.prog.mk>
