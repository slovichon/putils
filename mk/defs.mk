# $Id$

ROOT = ${.CURDIR}/${SYSROOT}
.if exists(${ROOT}/lib/obj)
LIBROOT = ${ROOT}/lib/obj
.else
LIBROOT = ${ROOT}/lib
.endif
