/* $Id$ */

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "util.h"

void
xwarn(char *fmt, ...)
{
	if (fmt == NULL)
		if (errno)
			warn(NULL);
		else
			/* ??? */
			warnx(NULL);
	else {
		va_list ap;
		
		va_start(ap, fmt);
		if (errno)
			vwarn(fmt, ap);
		else
			vwarnx(fmt, ap);
		va_end(ap);
	}
}

char *
join(char **s)
{
	size_t siz = 0;
	char *p, **t;

	for (t = s; *t != NULL; t++)
		/*
		 * Add 1+strlen for each, which takes care
		 * of NUL for the last argument.
		 */
		siz += strlen(*t) + 1;

	if ((p = malloc(siz)) == NULL)
		err(EX_OSERR, "malloc");
	p[0] = '\0';
	for (t = s; *t != NULL; t++) {
		strlcat(p, *t, siz);
		strlcat(p, " ", siz);
	}
	return (p);
}
