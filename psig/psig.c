/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/signalvar.h>

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <limits.h>
#include <kvm.h>
#include <sysexits.h>

#include "putils.h"
#include "util.h"

static		void doproc(char *);
static __dead	void usage(void);

static kvm_t *kd = NULL;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];

	if (argc < 2)
		usage();
	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL,
	     (char *)NULL, KVM_NO_FILES, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	while (*++argv != NULL)
		doproc(*argv);
	(void)kvm_close(kd);
	exit(EXIT_SUCCESS);
}

static void
doproc(char *s)
{
	int pcnt, i, blocked, wrote;
	struct kinfo_proc2 *kip;
	struct sigacts *sa;
	char **argv, *cmd;
	u_int32_t sig;
	ssize_t siz;
	pid_t pid;

	if (!parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}
	kip = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kip), &pcnt);
	if (kip == NULL)
		errx(EX_OSERR, "kvm_getproc2: %s", kvm_geterr(kd));
	else if (pcnt == 0) {
		errno = ESRCH;
		xwarn("cannot examine %s", s);
	} else {
		(void)printf("%d:\t", pid);
		if ((argv = kvm_getargv2(kd, kip, 0)) == NULL)
			(void)printf("%s\n", kip->p_comm);
		else {
			cmd = join(argv);
			(void)printf("%s\n", cmd);
			free(cmd);
		}
		sa = malloc(sizeof(*sa));
		for (i = 1; i < NSIG; i++) {
			sig = 1 << (i - 1);
			switch (i) {
			case SIGHUP:	(void)printf("HUP");	break;
			case SIGINT:	(void)printf("INT");	break;
			case SIGQUIT:	(void)printf("QUIT");	break;
			case SIGILL:	(void)printf("ILL");	break;
			case SIGABRT:	(void)printf("ABRT");	break;
			case SIGFPE:	(void)printf("FPE");	break;
			case SIGKILL:	(void)printf("KILL");	break;
			case SIGSEGV:	(void)printf("SEGV");	break;
			case SIGPIPE:	(void)printf("PIPE");	break;
			case SIGALRM:	(void)printf("ARLM");	break;
			case SIGTERM:	(void)printf("TERM");	break;
			case SIGSTOP:	(void)printf("STOP");	break;
			case SIGTSTP:	(void)printf("TSTP");	break;
			case SIGCONT:	(void)printf("CONT");	break;
			case SIGCHLD:	(void)printf("CHLD");	break;
			case SIGTTIN:	(void)printf("TTIN");	break;
			case SIGTTOU:	(void)printf("TTOU");	break;
			case SIGUSR1:	(void)printf("USR1");	break;
			case SIGUSR2:	(void)printf("USR2");	break;
#ifndef _POSIX_SOURCE
			case SIGTRAP:	(void)printf("TRAP");	break;
			case SIGEMT:	(void)printf("EMT");	break;
			case SIGBUS:	(void)printf("BUS");	break;
			case SIGSYS:	(void)printf("SYS");	break;
			case SIGURG:	(void)printf("URG");	break;
			case SIGIO:	(void)printf("IO");	break;
			case SIGXCPU:	(void)printf("XCPU");	break;
			case SIGXFSZ:	(void)printf("XFSZ");	break;
			case SIGVTALRM:	(void)printf("VTARLM");	break;
			case SIGPROF:	(void)printf("PROF");	break;
			case SIGWINCH:	(void)printf("WINCH");	break;
			case SIGINFO:	(void)printf("INFO");	break;
#endif
			default:
				warnx("%u: unsupported signal number", sig);
				goto nextsig;
				/* NOTREACHED */
			}

			(void)printf("\t");
			blocked = 0;
			wrote = 0;
			if (kip->p_sigmask & sig) {
				wrote += printf("blocked");
				blocked = 1;
			}
			if (kip->p_sigignore & sig)
				wrote += printf("%signore", blocked ? "," : "");
			else if (kip->p_sigcatch & sig)
				wrote += printf("%scaught", blocked ? "," : "");
			else
				wrote += printf("%sdefault", blocked ? "," : "");
			if (!(kip->p_sigignore & sig))
				(void)printf("%s\t%d", wrote > 8 ? "" : "\t",
				    kip->p_siglist & sig);

			(void)printf("\n");
nextsig:
			;
		}
		free(sa);
	}
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid ...\n", __progname);
	exit(EX_USAGE);
}
