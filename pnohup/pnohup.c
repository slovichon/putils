/* $Id$

#include <sys/param.h>
#include <sys/sysctl.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

void pnohup(char *);
void usage(void) __attribute__((__noreturn__));

int group = 0;
int kd;

int
main(int argc, char *argv[])
{
	char buf[LINE_MAX];
	int c;

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
	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL, (char *)NULL,
	     O_RDWR, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	while (*++argv != NULL)
		pnohup(*argv);
	(void)kvm_closefiles(kd);
	exit(EX_OK);
}

void
pnohup(char *s)
{
	struct kinfo_proc2 *kp;
	struct sigacts sa;
	int op, pcnt;
	pid_t pid;
	uid_t uid;

	if (!parsepid(s, &pid)) {
		xwarn();
		return;
	}
	op = KERN_PROC_PID;
	if (group)
		op = KERN_PROC_PGID;
	if ((kp = kvm_getproc2(kd, op, pid, sizeof(*kp), &pcnt)) == NULL)
		errx(EX_OSERR, "kvm_getproc2: %s", kvm_geterr(kd));
	if (pcnt == 0) {
		errno = ESRCH;
		xwarn("cannot examine %s", s);
	}
	uid = getuid();
	for (; pcnt--; kp++) {
		if (uid && kp->p_uid != uid) {
			errno = EPERM;
			xwarn("cannot examine %d", kp->p_pid);
			continue;
		}
		if (kvm_read(kd, (u_long)kp->p_sigacts, &sa, sizeof(sa)) != sizeof(sa))
			errx(EX_OSERR, "kvm_read: %s", kvm_geterr(kd));
		sa.ps_sigact[SIGHUP] = SIG_IGN;
		if (kvm_write(kd, (u_long)kp->p_sigacts, &sa, sizeof(sa)) != sizeof(sa))
			errx(EX_OSERR, "kvm_write: %s", kvm_geterr(kd));
		kp->p_sigignore |= SIGHUP;
		kp->p_sigcatch &= ~SIGHUP;
		kp->p_siglist &= ~SIGHUP;
	}
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-g] pid ...", __progname);
	exit(EX_USAGE);
}
