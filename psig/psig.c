/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>

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
static		int  sigcmp(const void *, const void *);
static __dead	void usage(void);

static kvm_t *kd = NULL;

static struct sig {
	int	 sig_num;
	char	*sig_nam;
} sigs[] = {
	{ SIGHUP,	"HUP" },
	{ SIGINT,	"INT" },
	{ SIGQUIT,	"QUIT" },
	{ SIGILL,	"ILL" },
	{ SIGABRT,	"ABRT" },
	{ SIGFPE,	"FPE" },
	{ SIGKILL,	"KILL" },
	{ SIGSEGV,	"SEGV" },
	{ SIGPIPE,	"PIPE" },
	{ SIGALRM,	"ARLM" },
	{ SIGTERM,	"TERM" },
	{ SIGSTOP,	"STOP" },
	{ SIGTSTP,	"TSTP" },
	{ SIGCONT,	"CONT" },
	{ SIGCHLD,	"CHLD" },
	{ SIGTTIN,	"TTIN" },
	{ SIGTTOU,	"TTOU" },
	{ SIGUSR1,	"USR1" },
	{ SIGUSR2,	"USR2" }

#ifndef _POSIX_SOURCE
				,
	{ SIGTRAP,	"TRAP" },
	{ SIGEMT,	"EMT" },
	{ SIGBUS,	"BUS" },
	{ SIGSYS,	"SYS" },
	{ SIGURG,	"URG" },
	{ SIGIO,	"IO" },
	{ SIGXCPU,	"XCPU" },
	{ SIGXFSZ,	"XFSZ" },
	{ SIGVTALRM,	"VTARLM" },
	{ SIGPROF,	"PROF" },
	{ SIGWINCH,	"WINCH" },
	{ SIGINFO,	"INFO" }
#endif
};
#define NSIGS (sizeof(sigs) / sizeof(sigs[0]))

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];

	if (argc < 2)
		usage();
	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL,
	     (char *)NULL, KVM_NO_FILES, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	qsort(sigs, NSIGS, sizeof(sigs[0]), sigcmp);
	while (*++argv != NULL)
		doproc(*argv);
	(void)kvm_close(kd);
	exit(EXIT_SUCCESS);
}

static int
sigcmp(const void *a, const void *b)
{
	struct sig *x, *y;

	x = (struct sig *)a;
	y = (struct sig *)b;

	if (x->sig_num < y->sig_num)
		return 1;
	else if (x->sig_num > y->sig_num)
		return -1;
	else
		return 0;
}

static void
doproc(char *s)
{
	struct kinfo_proc2 *kip;
	char **argv, *cmd;
	int pcnt, i;
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
		(void)printf("%d:  ", pid);
		if ((argv = kvm_getargv2(kd, kip, 0)) == NULL)
			(void)printf("%s\n", kip->p_comm);
		else {
			cmd = join(argv);
			(void)printf("%s\n", cmd);
			free(cmd);
		}
		for (i = 0; i < NSIGS; i++) {
		}
	}
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid ...\n", __progname);
	exit(EX_USAGE);
}
