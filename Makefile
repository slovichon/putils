# $Id$

SYSROOT = .

.include "mk/defs.mk"

SUBDIRS += lib pargs pcred psig ptree pwait pwdx

.include "mk/subdir.mk"

build: clean obj depend all
