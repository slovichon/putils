/* $Id$ */

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "putils.h"
#include "util.h"

static		void doproc(char *);
static __dead	void usage(void);

static int		nev = 0;
static int 		kd = -1;
static int 		verbose = 0;
static struct kevent	kev;

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
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	if ((kd = kqueue()) == -1)
		err(EX_OSERR, "kqueue");
	while (*argv != NULL)
		doproc(*argv++);
	while (kevent(kd, (struct kevent *)NULL, 0, &kev, 1,
	    (const struct timespec *)NULL) != -1) {
		if (verbose) {
			(void)printf("%d: status %d\n", kev.ident,
			    kev.data);
		} else
			(void)printf("%d: terminated\n", kev.ident);
	}
	(void)close(kd);
	exit(EXIT_SUCCESS);
}

static void
doproc(char *s)
{
	pid_t pid;

	if (!parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}
	(void)memset(&kev, 0, sizeof(kev));
	kev.ident = pid;
	kev.flags = EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT;
	kev.filter = EVFILT_PROC;
	kev.fflags = NOTE_EXIT;
	if (kevent(kd, &kev, 1, NULL, 0, NULL) == -1) {
		if (errno == ESRCH) {
			warn("cannot examine %s", s);
			return;
		} else
			err(EX_OSERR, "kevent: %d", pid);
	}
	nev++;
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-v] pid|core ...\n", __progname);
	exit(EX_USAGE);
}
