# $Id$

ROOT = ${.CURDIR}/${SYSROOT}
.if exists(${ROOT}/lib/obj)
LIBDIR = ${ROOT}/lib/obj
.else
LIBDIR = ${ROOT}/lib
.endif
