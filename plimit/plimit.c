/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <util.h>

#define PID_MAX		INT_MAX

#define RSET_CUR	1
#define RSET_MAX	2

enum type {
	T_BLKS, T_TIME, T_DISC, T_BYTE
};

struct resource {
	const char	*re_name;
	int		 re_val;
	char		 re_flag;
	enum type	 re_type;
	const char	*re_units;

	rlim_t		 re_newcur;
	rlim_t		 re_newmax;
	int		 re_set;
};

int		plimit(const char *);
__dead void	usage(void);
int		disp(enum type, rlim_t);
int		prunits(const struct resource *);
void		parse(struct resource *, char *);
rlim_t		parse_time(char *);

struct resource res[] = {
	/* name		val		flag type	units */
	{ "core",	RLIMIT_CORE,	'c', T_BLKS },
	{ "data",	RLIMIT_DATA,	'd', T_BYTE },
	{ "file",	RLIMIT_FSIZE,	'f', T_BLKS },
	{ "mlock",	RLIMIT_MEMLOCK,	'l', T_BYTE },
	{ "rss",	RLIMIT_RSS,	'm', T_BYTE },
	{ "nfd",	RLIMIT_NOFILE,	'n', T_DISC,	"descriptors" },
	{ "stack",	RLIMIT_STACK,	's', T_BYTE },
	{ "time",	RLIMIT_CPU,	't', T_TIME },
	/* { "vmemory",	RLIMIT_VMEM,	'v', T_BYTE }, */
	{ NULL }
};

kvm_t		*kd = NULL;
int		 humansiz = 0;
int		 doset = 0;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];
	struct resource *re;
	int c, status;

	while ((c = getopt(argc, argv, "c:d:f:hl:m:n:s:t:v:")) != -1)
		switch (c) {
		case 'h':
			humansiz = 1;
			break;
		default:
			for (re = res; re->re_name != NULL; re++)
				if (re->re_flag == c) {
					parse(re, optarg);
					doset = 1;
					break;
				}
			if (re->re_name == NULL)	/* Flag not found. */
				usage();
			break;
		}
	argv += optind;
	if (*argv == NULL)
		usage();
	if ((kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, buf)) ==
	    NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	status = 0;
	while (*argv != NULL)
		status |= plimit(*argv++);
	(void)kvm_close(kd);
	exit(status ? EX_UNAVAILABLE : EX_OK);
}

int
plimit(const char *s)
{
	struct kinfo_proc2 *kp;
	struct resource *re;
	const char *errstr;
	struct rlimit *rl;
	int pcnt, written;
	struct plimit pl;
	pid_t pid;

	errstr = NULL;
	pid = strtonum(s, 0, PID_MAX, &errstr);
	if (errstr != NULL) {
		warnx("%s: %s", s, errstr);
		return (1);
	}
	kp = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kp), &pcnt);
	if (kp == NULL || pcnt == 0) {
		if (!errno)
			errno = ESRCH;
		warn("%s", s);
		errno = 0;
		return (1);
	}
	if (kvm_read(kd, (u_long)kp->p_limit, &pl,
	    sizeof(pl)) != sizeof(pl)) {
		warn("kvm_read");
		return (1);
	}
	if (doset) {
		for (re = res; re->re_name != NULL; re++) {
			rl = &pl.pl_rlimit[re->re_val];
			if (re->re_set & RSET_CUR)
				rl->rlim_cur = re->re_newcur;
			if (re->re_set & RSET_MAX)
				rl->rlim_max = re->re_newmax;
		}
		if (kvm_write(kd, (u_long)kp->p_limit, &pl,
		    sizeof(pl)) != sizeof(pl)) {
			warn("kvm_write");
			return (1);
		}
	} else {
		(void)printf("%d:\n"
		    "   resource\t\t current\t maximum\n", pid);
		for (re = res; re->re_name != NULL; re++) {
			written = prunits(re);
			printf("\t%s", written > 15 ? "" : "\t");
			rl = &pl.pl_rlimit[re->re_val];
			written = disp(re->re_type, rl->rlim_cur);
			printf("\t%s", written > 7 ? "" : "\t");
			(void)disp(re->re_type, rl->rlim_max);
			(void)printf("\n");
		}
	}
	return (0);
}

int
prunits(const struct resource *re)
{
	int written;

	written = printf("  %s(", re->re_name);
	switch (re->re_type) {
	case T_TIME:
		written += printf("seconds");
		break;
	case T_DISC:
		written += printf("%s", re->re_units);
		break;
	case T_BYTE:
	case T_BLKS:
		/* XXX:  inspect value for mbytes, gbytes, etc. */
		if (humansiz || re->re_type == T_BYTE)
			written += printf("kbytes");
		else
			written += printf("blocks");
		break;
	}
	written += printf(")");
	return (written);
}

int
disp(enum type type, rlim_t lim)
{
	int written, hlen;
	long blksiz;

	written = 0;
	if (lim == RLIM_INFINITY)
		written += printf("unlimited");
	else
		switch (type) {
		case T_TIME:
		case T_DISC:
			written += printf("%lld", lim);
			break;
		case T_BLKS:
			if (humansiz) {
				getbsize(&hlen, &blksiz);
				lim *= blksiz;
				written += printf("%lld", lim);
			} else
				written += printf("%lld", lim);
			break;
		case T_BYTE:
			/* XXX:  fmt_scaled. */
			written += printf("%lld", lim / 1024);
			break;
		}
	return (written);
}

void
parse(struct resource *re, char *p)
{
	long long l;
	long blksiz;
	char *s, *t;
	int hlen;

	for (s = p; s != NULL; s = t) {
		t = strtok(s, ",");
		switch (re->re_type) {
		case T_TIME:
			l = parse_time(s);
			break;
		case T_BLKS:
		case T_BYTE:
			if (!scan_scaled(s, &l))
				err(1, "%s: invalid size", s);
			if (re->re_type == T_BLKS) {
				/* `l' is in bytes, need blocks. */
				getbsize(&hlen, &blksiz);
				l /= blksiz;
			}
			break;
		case T_DISC:
			if ((l = strtoull(s, NULL, 0)) < 0)
				err(1, "%s: invalid value", s);
			break;
		}
		if (t)
			re->re_newcur = (rlim_t)l;
		else
			re->re_newmax = (rlim_t)l;
	}
}

rlim_t
parse_time(char *p)
{
	char *t, *s, *hr, *min, *sec;
	rlim_t lim = 0LL;
	int sawcolon;

	sawcolon = 0;
	hr = min = sec = NULL;
	for (s = t = p; *s != '\0'; s++) {
		switch (*s) {
		case '0': case '1': case '2':
		case '3': case '4': case '5':
		case '6': case '7': case '8':
		case '9':
			lim = (lim * 10) + (*s - '0');
			break;
		case ':':
			if (sawcolon || hr != NULL || min != NULL ||
			    sec != NULL)
				goto badtime;
			lim *= 60;
			sawcolon = 1;
			break;
		case 'h':
			if (hr != NULL || sawcolon)
				goto badtime;
			lim = 0LL;
			*s = '\0';
			hr = t;
			t = s + 1;
			break;
		case 'm':
			if (min != NULL || sawcolon)
				goto badtime;
			lim = 0LL;
			*s = '\0';
			min = t;
			t = s + 1;
			break;
		case 's':
			if (sec != NULL || sawcolon)
				goto badtime;
			lim = 0LL;
			*s = '\0';
			sec = t;
			t = s + 1;
			break;
		default:
			goto badtime;
		}
	}
	if (s == p)
		errx(1, "no time specified");
	if (sec != NULL && lim)
		goto badtime;
	if (hr != NULL)
		lim += 60 * 60 * (rlim_t)strtoul(hr, NULL, 0);
	if (min != NULL)
		lim += 60 * (rlim_t)strtoul(min, NULL, 0);
	if (sec != NULL)
		lim += (rlim_t)strtoul(sec, NULL, 0);
	return (lim);

badtime:
	errx(1, "%s: invalid time", p);
	/* NOTREACHED */
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: "
	    "\t%s [-c core] [-d data] [-f file] [-l mlock] [-m rss]\n"
	    "\t       [-n nfd] [-s stack] [-t time] pid ...\n"
	    "\t%s [-h] pid ...\n",
	    __progname, __progname);
	exit(EX_USAGE);
}
