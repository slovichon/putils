/* $Id$ */

#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

int verbose = 0;

static		void doproc(char *);
static __dead	void usage(void);

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

	while (*argv != NULL)
		doproc(*argv);
	exit(EXIT_SUCCESS);
}

static void
doproc(char *s)
{
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-v] pid|core ...\n", __progname);
	exit(EX_USAGE);
}
