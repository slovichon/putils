/* $Id$ */

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

#include "pstack.h"
#include "putils.h"
#include "util.h"

static		void doproc(char *);
static __dead	void usage(void);

static kvm_t *kd;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];

	if (argc < 2)
		usage();
	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL,
	     (char *)NULL, O_RDONLY, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	while (*++argv != NULL)
		doproc(*argv);
	(void)kvm_close(kd);
	exit(EXIT_SUCCESS);
}

static void
doproc(char *s)
{
	struct kinfo_proc2 *kip;
	unsigned long sp;
	char **argv, *p;
	pid_t pid;
	int pcnt;

	if (!parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}
	kip = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kip), &pcnt);
	if (kip == NULL)
		warnx("kvm_getproc2: %s", kvm_geterr(kd));
	else if (pcnt == 0) {
		errno = ESRCH;
		xwarn("cannot examine %s", s);
	} else {
		if ((p = getsp(kd, kip, &sp)) != NULL) {
			warnx("cannot examine %s: %s", s, p);
			return;
		}
		if ((argv = kvm_getargv2(kd, kip, 0)) == NULL)
			(void)printf("%d:\t%s\n", pid, kip->p_comm);
		else {
			p = join(argv);
			(void)printf("%d:\t%s\n", pid, p);
			free(p);
		}
		while (sp != 0) {

		}
	}
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid|core ...\n", __progname);
	exit(EX_USAGE);
}
