/* $Id$ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include <vis.h>

#include "putils.h"
#include "util.h"

#define PRARG (1<<0)
#define PRENV (1<<1)
#define PRAUX (1<<2)

static		void doproc(char *);
static		void prarg(char *, pid_t, struct kinfo_proc2 *);
static		void praux(char *, pid_t, struct kinfo_proc2 *);
static		void prenv(char *, pid_t, struct kinfo_proc2 *);
static __dead	void usage(void);

static kvm_t *kd = NULL;
static int print = PRARG;
static int force = 0;
static int ascii = 0;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];
	int ch;

	while ((ch = getopt(argc, argv, "aceFx")) != -1) {
		switch (ch) {
		case 'A':
			print = PRARG | PRENV | PRAUX;
			break;
		case 'a':
			print = PRARG;
			break;
		case 'c':
			ascii = 1;
			break;
		case 'e':
			print = PRENV;
			break;
		case 'F':
			force = 1;
			break;
		case 'x':
			print = PRAUX;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();

	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL, (char *)NULL,
	     KVM_NO_FILES, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	while (*argv != NULL)
		doproc(*argv);
	kvm_close(kd);
	exit(0);
}

static void
doproc(char *s)
{
	char *lc, *curlc;
	struct kinfo_proc2 *kip;
	pid_t pid;
	int pcnt, conv, cat;

	/* shutup gcc */
	cat = conv = 0;
	lc = curlc = NULL;

	if (!parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}

	kip = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kip), &pcnt);
	if (kip == NULL)
		warnx("kvm_getproc2: %s", kvm_geterr(kd));
	else if (pcnt == 0) {
		errno = ENOENT;
		warnx("cannot examine %s", s);
	} else {
		if (!ascii) {
			if (0) {
				curlc = setlocale(cat, lc);
				conv = 1;
			}
		}
		if (print & PRARG)
			prarg(s, pid, kip);
		if (print & PRENV)
			prenv(s, pid, kip);
		if (print & PRAUX)
			praux(s, pid, kip);
		if (conv)
			(void)setlocale(cat, curlc);
	}
}

char *
xstrvisdup(const char *s, int flags)
{
	size_t siz;
	char *p = "";

	siz = 1 + strnvis(p, s, 0, flags);
	if ((s = malloc(siz)) == NULL)
		err(EX_OSERR, NULL);
	(void)strnvis(p, s, siz, flags);
	return (p);
}

static void
prarg(char *arg, pid_t pid, struct kinfo_proc2 *kip)
{
	char *s, **argv;
	int i;

	if ((argv = kvm_getargv2(kd, kip, 0)) == NULL) {
		warnx("cannot examine %s: %s", arg, kvm_geterr(kd));
		return;
	}
	(void)printf("%d:\n", pid);
	for (i = 0; *argv != NULL; i++) {
		s = xstrvisdup(*argv++, VIS_OCTAL);
		(void)printf("argv[%d]: %s\n", i, s);
		free(s);
	}
	(void)printf("\n");
}

static void
prenv(char *arg, pid_t pid, struct kinfo_proc2 *kip)
{
	char *s, **envp;
	int i;

	if ((envp = kvm_getenvv2(kd, kip, 0)) == NULL) {
		warnx("cannot examine %s: %s", arg, kvm_geterr(kd));
		return;
	}
	(void)printf("%d:\n", pid);
	for (i = 0; *envp != NULL; i++) {
		s = xstrvisdup(*envp, VIS_OCTAL);
		(void)printf("envp[%d]: %s\n", i, *envp++);
		free(s);
	}
	(void)printf("\n");
}

static void
praux(char *arg, pid_t pid, struct kinfo_proc2 *kip)
{
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: [-aceFx] %s pid|core ...\n", __progname);
	exit(0);
}
