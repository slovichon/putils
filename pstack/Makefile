# $Id$

PROG=		pstack
SRCS=		pstack.c ${MACHINE_ARCH}.c
LDADD+=		-lkvm
DPADD+=		${LIBKVM}
BINMODE=	2555
BINGRP=		kmem
CFLAGS+=	-I${.CURDIR}/../lib

.include <bsd.prog.mk>
