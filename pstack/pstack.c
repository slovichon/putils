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

#include "pathnames.h"
#include "pstack.h"
#include "putils.h"
#include "symtab.h"
#include "util.h"

static void doproc(char *);
static void usage(void) __attribute__((__noreturn__));

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
	char *p, fil[MAXPATHLEN], **argv, *t;
	struct kinfo_proc2 *kip;
	struct symtab *st;
	unsigned long sp;
	const char *buf;
	int pcnt, n;
	pid_t pid;

	if ((p = getpidpath(s, &pid, 0)) == NULL) {
		xwarn("cannot examine %s", s);
		return;
	}
	(void)snprintf(fil, sizeof(fil), "%s%s", p, _RELPATH_FILE);
	free(p);
	kip = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kip), &pcnt);
	if (kip == NULL)
		warnx("kvm_getproc2: %s", kvm_geterr(kd));
	else if (pcnt == 0) {
		errno = ESRCH;
		xwarn("cannot examine %s", s);
	} else {
		if ((buf = getsp(kd, kip, &sp)) != NULL) {
			warnx("cannot examine %s: %s", s, buf);
			return;
		}
		if ((st = symtab_open(fil)) == NULL) {
			xwarn("cannot examine %s", s);
			return;
		}
		if ((argv = kvm_getargv2(kd, kip, 0)) == NULL)
			(void)printf("%d:\t%s\n", pid, kip->p_comm);
		else {
			t = join(argv);
			(void)printf("%d:\t%s\n", pid, t);
			free(t);
		}
		while (sp != 0) {
			/* Print frame. */
			n = printf(" %lx", sp);
			(void)printf(" %s()\n", symtab_getsym(st, sp));
			sp = 0;
		}
		symtab_close(st);
	}
}

static void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid|core ...\n", __progname);
	exit(EX_USAGE);
}
