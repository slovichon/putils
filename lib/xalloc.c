/* $Id$ */

#include <err.h>
#include <string.h>
#include <sysexits.h>

#include "xalloc.h"

char *
xstrdup(const char *s)
{
	char *p;

	if ((p = strdup(s)) == NULL)
		err(EX_OSERR, "strdup");
	return (p);
}
