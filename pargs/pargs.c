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

#define PID_MAX	INT_MAX

#define PRARG (1<<0)
#define PRENV (1<<1)

int		 pargs(char *);
void		 prarg(char *, pid_t, struct kinfo_proc2 *);
void		 prenv(char *, pid_t, struct kinfo_proc2 *);
char		*xstrvisdup(const char *, int);
__dead void	 usage(void);

kvm_t		*kd = NULL;
int		  print = PRARG;
int		 ascii = 0;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];
	int ch, status;

	while ((ch = getopt(argc, argv, "Aace")) != -1) {
		switch (ch) {
		case 'A':
			print = PRARG | PRENV;
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
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argv += optind;
	if (argv == NULL)
		usage();

	if ((kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES,
	     buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	status = 0;
	while (*argv != NULL)
		status |= pargs(*argv++);
	(void)kvm_close(kd);
	exit(status ? EX_UNAVAILABLE : EX_OK);
}

int
pargs(char *s)
{
	struct kinfo_proc2 *kp;
	int pcnt, conv, cat;
	const char *errstr;
	char *lc, *curlc;
	pid_t pid;

	/* shutup gcc */
	cat = 0;
	lc = curlc = NULL;

	pid = strtonum(s, 0, PID_MAX, &errstr);
	if (errstr != NULL) {
		warnx("%s: %s", s, errstr);
		return (1);
	}

	kp = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kp), &pcnt);
	if (kp == NULL)
		errx(EX_OSERR, "kvm_getproc2: %s", kvm_geterr(kd));
	if (pcnt == 0) {
		if (!errno)
			errno = ESRCH;
		warn("%s", s);
		return (1);
	}
	conv = 0;
	if (!ascii) {
#if 0
		cat = 0;
		lc = 0;
		curlc = setlocale(cat, lc);
		conv = 1;
#endif
	}
	if (print & PRARG)
		prarg(s, pid, kp);
	if (print & PRENV)
		prenv(s, pid, kp);
	/* Reset locale. */
	if (conv)
		(void)setlocale(cat, curlc);
	return (0);
}

char *
xstrvisdup(const char *s, int flags)
{
	size_t siz;
	char *p = "";

	siz = 1 + strnvis(p, s, 0, flags);
	if ((p = malloc(siz)) == NULL)
		err(EX_OSERR, NULL);
	(void)strnvis(p, s, siz, flags);
	return (p);
}

void
prarg(char *arg, pid_t pid, struct kinfo_proc2 *kp)
{
	char *s, **argv;
	int i;

	if ((argv = kvm_getargv2(kd, kp, 0)) == NULL) {
		warnx("cannot examine %s: %s", arg, kvm_geterr(kd));
		return;
	}
	(void)printf("%d:\n", pid);
	for (i = 0; *argv != NULL; i++) {
		s = xstrvisdup(*argv++, VIS_OCTAL);
		(void)printf("argv[%d]: %s\n", i, s);
		free(s);
	}
}

void
prenv(char *arg, pid_t pid, struct kinfo_proc2 *kp)
{
	char *s, **envp;
	int i;

	if ((envp = kvm_getenvv2(kd, kp, 0)) == NULL) {
		warnx("cannot examine %s: %s", arg, kvm_geterr(kd));
		return;
	}
	(void)printf("%d:\n", pid);
	for (i = 0; *envp != NULL; i++) {
		s = xstrvisdup(*envp, VIS_OCTAL);
		(void)printf("envp[%d]: %s\n", i, *envp++);
		free(s);
	}
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: [-Aace] %s pid | core ...\n",
	    __progname);
	exit(EX_USAGE);
}
