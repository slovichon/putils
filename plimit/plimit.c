/* $Id$ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "putils.h"
#include "util.h"

static void doproc(char *);
static void usage(void) __attribute__((__noreturn__));

static kvm_t *kd = NULL;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];
	int c;

	while ((c = getopt(argc, argv, "")) != -1)
		switch (c) {
		default:
			usage();
			/* NOTREACHED */
		}
	argv += optind;
	if (argc < 1)
		usage();
	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL, (char *)NULL,
	     O_RDONLY, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	while (*argv != NULL)
		doproc(*argv);
	(void)kvm_close(kd);
	exit(EXIT_SUCCESS);
}

static void
doproc(char *s)
{
	struct kinfo_proc2 *kip;
	struct plimit *pl;
	pid_t pid;
	int pcnt;

	if (!parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}
	kip = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kip), &pcnt);
	if (kip == NULL)
		xwarn("cannot examine %s", s);
	else if (pcnt == 0) {
		errno = ESRCH;
		xwarn("cannot examine %s", s);
	} else {
		if ((pl = malloc(sizeof(*pl))) == NULL) {
			xwarn("cannot examine %s", s);
			return;
		}
		if (kvm_read(kd, (u_long)kip->p_limit, pl, sizeof(*pl)) !=
		    sizeof(*pl)) {
			xwarn("cannot examine %s", s);
			return;
		}
		(void)printf("%d:\n", pid);

		/* Maximum resident set size. */
		//(void)printf("m: %d",);

#if 0
	-c      the maximum size of core files created
	-d      the maximum size of a process data segment
	-f      the maximum size of files created by the shell
	-l      the maximum size a process may lock into memory
	-n      the maximum number of open file descriptors
	-p      the pipe buffer size
	-s      the maximum stack size
	-t      the maximum amount of cpu time in seconds
	-u      the maximum number of user processes
	-v      the size of virtual memory

	u_int64_t p_uru_ixrss;		/* LONG: integral shared memory size. */
	u_int64_t p_uru_idrss;		/* LONG: integral unshared data ". */
	u_int64_t p_uru_isrss;		/* LONG: integral unshared stack ". */
#endif


		(void)printf("\n");
	}
}

static void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s\n", __progname);
	exit(EX_USAGE);
}
