/* $Id$ */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/signalvar.h>

#include <elf_abi.h>
#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include "putils.h"
#include "util.h"

static		void doproc(char *);
static		void prhandler(struct kinfo_proc2 *, int);
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
	int pcnt, i, blocked, wrote, hasprocfs;
	char **argv, *cmd, *p, fil[MAXPATHLEN];
	struct kinfo_proc2 *kip;
	struct sigacts *sa;
	u_int32_t sig;
	Elf_Ehdr hdr;
	FILE *binfp;
	pid_t pid;

	/* shutup gcc */
	binfd = -1;

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
		if (hasprocfs) {
			(void)snprintf(fil, sizeof(fil), "%s%s", p,
			    _RELPATH_FILE);
			if ((binfp = fopen(fil, "r")) != NULL) {
				if (fread(&hdr, 1, sizeof(hdr), binfp) == 1) {
					
				}
			}
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

			if (kip->p_sigcatch & sig)
				prhandler(kip, i);

			/* XXX: show flags */

			(void)printf("\n");
nextsig:
			;
		}
		free(sa);
		if (binfp != NULL)
			(void)fclose(binfp);
	}
}

static void
prhandler(struct kinfo_proc2 *kip, int i)
{
	struct sigacts sa;
	uid_t uid;

	uid = getuid();
	if (uid != 0 && uid != kip->p_uid)
		return;
	if (kvm_read(kd, (u_long)kip->p_sigacts, &sa, sizeof(sa)) !=
	    sizeof(sa)) {
warnx("%s", kvm_geterr(kd));
		return;
}
	(void)printf("\t%p\n", sa.ps_sigact);
}



static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid ...\n", __progname);
	exit(EX_USAGE);
}
