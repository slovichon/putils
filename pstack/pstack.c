/* $Id$ */

#include <stdio.h>
#include <sysexits.h>
#include <stdlib.h>

#include "putils.h"

static int force = 0;

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;
	int c;

	while ((c = getopt(argc, argv, "F")) != -1) {
		switch (c) {
		case 'F':
			force = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argv += optind;

	while (*argv != NULL)
		doproc(*argv);
	exit(EXIT_SUCCESS);
}

static void
doproc(char *s)
{
	char *path, fil[MAXPATHLEN];
	pid_t pid;
	int memfd = -1, binfd = -1;

	if ((path = getpidpath(s, &pid, 0)) == NULL)
		goto bail;
	(void)snprintf(fil, sizeof(fil), "%s/mem", path);
	if ((memfd = open(fil, O_RDONLY)) == -1)
		goto bail;
	(void)snprintf(fil, sizeof(fil), "%s/mem", path);
	if ((binfd = open(fil, O_RDONLY)) == -1)
		goto bail;
	(void)close(memfd);
	(void)close(binfd);
bail:
	if (memfd != -1)
		(void)close(memfd);
	if (binfd != -1)
		(void)close(memfd);
	xwarn("cannot examine %s", s);
	free(path);
}

__dead static void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: [-F] %s\n", __progname);
	exit(EX_USAGE);
}
