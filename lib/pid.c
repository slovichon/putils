/* $Id$ */

#include <sys/param.h>
#include <sys/mount.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "pathnames.h"
#include "putils.h"
#include "xalloc.h"

#define PID_MAX INT_MAX

int
parsepid(char *s, pid_t *pid)
{
	char *p, fil[MAXPATHLEN];
	struct statfs fst;
	int ch, isnum;
	FILE *fp;
	long l;

	isnum = 1;
	for (p = s; *p != '\0'; p++)
		if (!isdigit(*p)) {
			isnum = 0;
			break;
		}

	if (p == s)
		return (0);

	if (isnum) {
		l = strtoul(s, NULL, 10);
		if (l <= PID_MAX) {
			*pid = (pid_t)l;
			return (1);
		} else
			return (0);
	}

	/* Try /proc/<pid>/status */
	if (statfs(s, &fst) == -1)
		return (0);
	if (strcmp(fst.f_fstypename, MOUNT_PROCFS) != 0)
		return (0);
	(void)snprintf(fil, sizeof(fil), "%s/status", s);
	if ((fp = fopen(fil, "r")) == NULL)
		return (0);
	/* This worked, use pid from pid field. */
	while (!isdigit(ch = fgetc(fp)) && ch != EOF)
		;
	for (l = 0; isdigit(ch); ch = fgetc(fp))
		l = l * 10 + (ch - '0');
	(void)fclose(fp);

	if (l <= PID_MAX) {
		*pid = (pid_t)l;
		return (1);
	} else
		return (0);
}

char *
getpidpath(char *s)
{
	char *p, fil[MAXPATHLEN];
	struct statfs fst;
	int isnum;

	isnum = 1;
	for (p = s; *p != '\0'; p++)
		if (!isdigit(*p)) {
			isnum = 0;
			break;
		}

	if (p == s)
		return (NULL);
	if (isnum) {
		snprintf(fil, sizeof(fil), "%s/%s", _PATH_PROC, s);
		s = fil;
	}
	if (statfs(s, &fst) == -1) {
		if (errno != ENOENT)
			warn("%s", s);
		return (NULL);
	}
	if (strcmp(fst.f_fstypename, MOUNT_PROCFS) != 0) {
		/* If we built the path, /proc is not mounted. */
		if (isnum)
			errx(EX_UNAVAILABLE, "/proc: not mounted");
		return (NULL);
	}
	return (xstrdup(s));
}
