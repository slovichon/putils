/* $Id$ */

#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>

int verbose = 0;

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "v")) != -1) {
		switch (ch) {
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argv += optind;

	for (; *argv != NULL; argv++)
	exit(EXIT_SUCCESS);
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-v] pid|core ...\n");
	exit(EX_USAGE);
}
