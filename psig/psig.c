/* $Id$ */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/signalvar.h>

#include <elf_abi.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include "pathnames.h"
#include "putils.h"
#include "symtab.h"
#include "util.h"

static		void doproc(char *);
static		void prhandler(struct kinfo_proc2 *, struct symtab *, int);
static __dead	void usage(void);

static int names = 0;
static kvm_t *kd = NULL;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];
	int c;

	while ((c = getopt(argc, argv, "n")) != -1) {
		switch (c) {
		case 'n':
			names = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argv += optind;

	if (*argv == NULL)
		usage();
	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL,
	     (char *)NULL, O_RDONLY, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	while (*argv != NULL)
		doproc(*argv++);
	(void)kvm_close(kd);
	exit(EXIT_SUCCESS);
}

static void
doproc(char *s)
{
	int pcnt, i, blocked, wrote, hasprocfs;
	char **argv, *cmd, *p, fil[MAXPATHLEN];
	struct kinfo_proc2 *kip;
	struct sigacts *sa;
	struct symtab *st;
	u_int32_t sig;
	pid_t pid;

	/*
	 * If /proc is not mounted, getpidpath() will fail.
	 * That does not mean that the program has to quit
	 * though; behavior that requires things in
	 * /proc/pid/ can just be ignored.
	 */
	hasprocfs = 1;
	if ((p = getpidpath(s, &pid, P_NODIE)) == NULL) {
		if (!parsepid(s, &pid)) {
			xwarn("cannot examine %s", s);
			return;
		}
		hasprocfs = 0;
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
		st = NULL;
		if (hasprocfs) {
			(void)snprintf(fil, sizeof(fil), "%s%s", p,
			    _RELPATH_FILE);
			st = symtab_open(fil);
			/* Silently ignore failures. */
		}
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

			if (names && (kip->p_sigcatch & sig) && st != NULL)
				prhandler(kip, st, i);

			/* XXX: show flags */

			(void)printf("\n");
nextsig:
			;
		}
		free(sa);
		if (st != NULL)
			symtab_close(st);
	}
}

static void
prhandler(struct kinfo_proc2 *kip, struct symtab *st, int i)
{
	struct sigacts sa;
	u_long haddr;
	uid_t uid;

	uid = getuid();
	if (uid != 0 && uid != kip->p_uid)
		return;
	if (kvm_read(kd, (u_long)kip->p_sigacts, &sa, sizeof(sa)) !=
	    sizeof(sa))
		return;
	haddr = (u_long)sa.ps_sigact[i];
	(void)printf("\t%s", symtab_getsym(st,
	    (unsigned long)sa.ps_sigact[i]));
}



static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: [-n] %s pid ...\n", __progname);
	exit(EX_USAGE);
}
