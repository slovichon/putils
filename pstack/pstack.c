/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "pathnames.h"
#include "pstack.h"
#include "putils.h"
#include "symtab.h"
#include "util.h"

#define MAXDEPTH 100

int		 pstack(char *);
__dead void	 usage(void);

kvm_t		*kd;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];
	int status, c;

	while ((c = getopt(argc, argv, "")) != -1)
		switch (c) {
		default:
			usage();
			/* NOTREACHED */
		}
	argv += optind;
	if (*argv != NULL)
		usage();
	if ((kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY,
	    buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	status = 0;
	while (*argv != NULL)
		status |= pstack(*argv++);
	(void)kvm_close(kd);
	exit(status ? EX_UNAVAILABLE: EX_OK);
}

int
pstack(char *s)
{
	char *p, fil[MAXPATHLEN], **argv, **pp;
	struct kinfo_proc2 *kp;
	struct symtab *st;
	int error, pcnt, n, j;
	unsigned long sp;
	pid_t pid;

	error = 0;
	if ((p = getpidpath(s, &pid, 0)) == NULL) {
		warn("%s", s);
		return (1);
	}
	(void)snprintf(fil, sizeof(fil), "%s%s", p, _RELPATH_FILE);
	free(p);
	if ((kp = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kp),
	    &pcnt)) == NULL)
		errx(EX_OSERR, "kvm_getproc2: %s", kvm_geterr(kd));
	else if (pcnt == 0) {
		errno = ESRCH;
		warn("%s", s);
		return (1);
	}
	if (kill(pid, SIGSTOP) == -1) {
		warn("%s", s);
		return (1);
	}
	if ((sp = getsp(kp)) == 0) {
		errno = EFAULT; /* XXX */
		warn("%s", s);
		error = 1;
		goto done;
	}
	if ((st = symtab_open(fil)) == NULL) {
		warn("%s", s);
		error = 1;
		goto done;
	}
	if ((argv = kvm_getargv2(kd, kp, 0)) == NULL)
		(void)printf("%d:\t%s\n", pid, kp->p_comm);
	else {
		(void)printf("%d:\t", pid);
		for (pp = argv; *pp != NULL; pp++)
			(void)printf("%s%s", *pp,
			    pp[1] == NULL ? "" : " ");
		(void)printf("\n");
	}
	for (j = 0; j < MAXDEPTH && sp != 0; j++) {
		/* Print frame. */
		n = printf(" %lx", sp);
		(void)printf(" %s()\n", symtab_getsymname(st, sp));
		sp = 0;
	}
	symtab_close(st);
done:
	(void)kill(pid, SIGCONT);
	return (error);
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid ...\n", __progname);
	exit(EX_USAGE);
}
