# $Id$

SYSROOT = .
SUBDIRS += lib pargs pcred pwait pwdx

.include "mk/subdir.mk"

build: clean obj depend all
