/* $Id$ */

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

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
