/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/signalvar.h>
#include <sys/proc.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#define PID_MAX	INT_MAX

int		pnohup(char *);
__dead void	usage(void);

int		 group = 0;
kvm_t		*kd = NULL;

int
main(int argc, char *argv[])
{
	char buf[LINE_MAX];
	int c, status;

	while ((c = getopt(argc, argv, "g")) != NULL)
		switch (c) {
		case 'g':
			group = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argv += optind;
	if (*argv == NULL)
		usage();
	if ((kd = kvm_openfiles(NULL, NULL, NULL, O_RDWR, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	status = 0;
	while (*argv != NULL)
		status |= pnohup(*argv++);
	(void)kvm_close(kd);
	exit(status ? EX_UNAVAILABLE : EX_OK);
}

int
pnohup(char *s)
{
	struct kinfo_proc2 *kp;
	const char *errstr;
	struct sigacts sa;
	int op, pcnt;
	pid_t pid;
	uid_t uid;

	pid = strtonum(s, 0, PID_MAX, &errstr);
	if (errstr != NULL) {
		warnx("%s: %s", s, errstr);
		return (1);
	}
	if (group) {
		if (killpg(pid, SIGSTOP) == -1) {
			warn("kill %d", pid);
			return (1);
		}
		op = KERN_PROC_PGRP;
	} else {
		if (kill(pid, SIGSTOP) == -1) {
			warn("kill %d", pid);
			return (1);
		}
		op = KERN_PROC_PID;
	}
	if ((kp = kvm_getproc2(kd, op, pid, sizeof(*kp), &pcnt)) == NULL)
		errx(EX_OSERR, "kvm_getproc2: %s", kvm_geterr(kd));
	if (pcnt == 0) {
		errno = ESRCH;
		warn("%s", s);
		return (1);
	}
	uid = getuid();
	for (; pcnt--; kp++) {
		if (uid && kp->p_uid != uid) {
			errno = EPERM;
			warn("%d", kp->p_pid);
			continue;
		}
		if (kvm_read(kd, (u_long)kp->p_sigacts, &sa,
		    sizeof(sa)) != sizeof(sa))
			errx(EX_OSERR, "kvm_read: %s", kvm_geterr(kd));
		sa.ps_sigact[SIGHUP] = SIG_IGN;
		if (kvm_write(kd, (u_long)kp->p_sigacts, &sa,
		    sizeof(sa)) != sizeof(sa))
			errx(EX_OSERR, "kvm_write: %s", kvm_geterr(kd));

		kp->p_sigignore |= SIGHUP;
		if (kvm_write(kd, (u_long)kp->p_paddr +
		    offsetof(struct proc, p_sigignore), &kp->p_sigignore,
		    sizeof(kp->p_sigignore)) != sizeof(kp->p_sigignore))
			errx(EX_OSERR, "kvm_write: %s", kvm_geterr(kd));

		kp->p_sigcatch &= ~SIGHUP;
		if (kvm_write(kd, (u_long)kp->p_paddr +
		    offsetof(struct proc, p_sigcatch), &kp->p_sigcatch,
		    sizeof(kp->p_sigcatch)) != sizeof(kp->p_sigcatch))
			errx(EX_OSERR, "kvm_write: %s", kvm_geterr(kd));

		kp->p_siglist &= ~SIGHUP;
		if (kvm_write(kd, (u_long)kp->p_paddr +
		    offsetof(struct proc, p_siglist), &kp->p_siglist,
		    sizeof(kp->p_siglist)) != sizeof(kp->p_siglist))
			errx(EX_OSERR, "kvm_write: %s", kvm_geterr(kd));
	}
	return (0);
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-g] pid ...\n", __progname);
	exit(EX_USAGE);
}
