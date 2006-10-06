/* $Id$ */

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#define PID_MAX	INT_MAX

int		pwait(char *);
__dead void	usage(void);

int		nev = 0;
int 		kd = -1;
int 		verbose = 0;

int
main(int argc, char *argv[])
{
	int ch, status;
	struct kevent kev;

	while ((ch = getopt(argc, argv, "v")) != -1) {
		switch (ch) {
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
		}
	}
	argv += optind;

	if (*argv == NULL)
		usage();
	if ((kd = kqueue()) == -1)
		err(EX_OSERR, "kqueue");
	status = 0;
	while (*argv != NULL)
		status |= pwait(*argv++);
	for (; nev > 0 && kevent(kd, NULL, 0, &kev, 1, NULL) != -1;
	    nev--)
		if (verbose)
			(void)printf("%d: terminated, wait status "
			    "0x%.4x\n", kev.ident, kev.data);
	(void)close(kd);
	exit(status ? EX_UNAVAILABLE : EX_OK);
}

int
pwait(char *s)
{
	const char *errstr;
	struct kevent kev;
	pid_t pid;

	pid = strtonum(s, 0, PID_MAX, &errstr);
	if (errstr != NULL) {
		warnx("%s: %s", s, errstr);
		return (1);
	}
	(void)memset(&kev, 0, sizeof(kev));
	kev.ident = pid;
	kev.flags = EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT;
	kev.filter = EVFILT_PROC;
	kev.fflags = NOTE_EXIT;
	if (kevent(kd, &kev, 1, NULL, 0, NULL) == -1) {
		if (errno == ESRCH) {
			warn("%s", s);
			return (1);
		} else
			err(EX_OSERR, "kevent: %d", pid);
	}
	nev++;
	return (0);
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-v] pid ...\n", __progname);
	exit(EX_USAGE);
}
