/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#define PID_MAX	INT_MAX

struct prcred {
	pid_t	 pr_pid;
	uid_t	 pr_ruid;
	uid_t	 pr_euid;
	uid_t	 pr_svuid;
	gid_t	 pr_rgid;
	gid_t	 pr_egid;
	gid_t	 pr_svgid;
	short	 pr_ngroups;
	gid_t	*pr_groups;
};

int		pcred(char *);
int		fromlive(pid_t);
void		printproc(struct prcred *);
__dead void	usage(void);

int
main(int argc, char *argv[])
{
	int c, status;

	while ((c = getopt(argc, argv, "")) != -1)
		switch (c) {
		default:
			usage();
			/* NOTREACHED */
		}
	argv += optind;
	if (*argv == NULL)
		usage();
	status = 0;
	while (*argv != NULL)
		status |= pcred(*argv++);
	exit(status ? EX_UNAVAILABLE : EX_OK);
}

int
pcred(char *s)
{
	const char *errstr;
	pid_t pid;
	int fd;

#if 0
	if ((fd = open(s, O_RDONLY)) != -1)
		return (fromcore(fd));
#endif
	pid = strtonum(s, 0, PID_MAX, &errstr);
	if (errstr != NULL) {
		warnx("%s: %s", s, errstr);
		return (1);
	}
	return (fromlive(pid));
}

int
fromlive(pid_t pid)
{
	struct kinfo_proc2 kp;
	struct prcred prc;
	size_t siz;
	int mib[6];

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC2;
	mib[2] = KERN_PROC_PID;
	mib[3] = pid;
	mib[4] = sizeof(kp);
	mib[5] = 1;
	siz = sizeof(kp);
	if (sysctl(mib, 6, &kp, &siz, NULL, 0) == -1)
		err(EX_OSERR, "sysctl");
	if (!siz) {
		errno = ESRCH;
		warn("%d", pid);
		return (1);
	}
	prc.pr_pid	= pid;
	prc.pr_ruid	= kp.p_ruid;
	prc.pr_euid	= kp.p_uid;
	prc.pr_svuid	= kp.p_svuid;
	prc.pr_rgid	= kp.p_rgid;
	prc.pr_egid	= kp.p_gid;
	prc.pr_svgid	= kp.p_svgid;
	prc.pr_ngroups	= kp.p_ngroups;
	prc.pr_groups	= (gid_t *)&kp.p_groups;
	printproc(&prc);
	return (0);
}

#if 0
int
fromcore(int fd)
{
	struct kinfo_proc *kp;
	union {
		unsigned char *buf;
		struct coreseg *cs;
	} cs;
	struct prcred prc;
	struct core c;
	union {
		unsigned char buf[1024*100];
		struct user u;
	} u;
	int i;

	if (read(fd, &c, sizeof(c)) != sizeof(c))
		err(EX_OSERR, "read");
	if ((cs.buf = malloc(c.c_seghdrsize)) == NULL)
		err(EX_OSERR, "malloc");
	(void)lseek(fd, c.c_hdrsize, SEEK_SET);
	for (i = 0; i < c.c_nseg; i++) {
		if (read(fd, cs.buf, c.c_seghdrsize) != c.c_seghdrsize)
			err(EX_OSERR, "read");
		if (read(fd, &u.buf, cs.cs->c_size) != cs.cs->c_size)
			err(EX_OSERR, "read");
		printf("%d: ", i);
		printf("%s [%s]\n", u.u.u_kproc.kp_eproc.e_emul,
		    u.u.u_kproc.kp_eproc.e_login);
	}
#if 0
	if (read(fd, &u, sizeof(u)) != sizeof(u))
		err(EX_OSERR, "read");
	kp = &u.u_kproc;
	printproc(&prc);
#endif
	return (0);
}
#endif

void
printproc(struct prcred *pcr)
{
	int i;

	/*
	 * Output has the following format:
	 *
	 *	10252:  euid=xxx ruid=xxx suid=204336  e/r/sgid=2004
	 *		groups: 33552 46185 2004
	 *
	 *	10252:  e/r/suid=204336  e/r/sgid=2004
	 *		groups: 33552 46185 2004
	 */

	(void)printf("%d:\t", pcr->pr_pid);
	if (pcr->pr_euid == pcr->pr_ruid)
		(void)printf("e/");
	else
		(void)printf("euid=%d ", pcr->pr_euid);
	if (pcr->pr_ruid == pcr->pr_svuid)
		(void)printf("r/");
	else
		(void)printf("ruid=%d ", pcr->pr_ruid);
	(void)printf("suid=%d  ", pcr->pr_svuid);
	if (pcr->pr_egid == pcr->pr_rgid)
		(void)printf("e/");
	else
		(void)printf("egid=%d ", pcr->pr_egid);
	if (pcr->pr_rgid == pcr->pr_svgid)
		(void)printf("r/");
	else
		(void)printf("rgid=%d ", pcr->pr_rgid);
	(void)printf("sgid=%d\n", pcr->pr_svgid);
	(void)printf("\tgroups:");
	for (i = 0; i < pcr->pr_ngroups; i++)
		(void)printf(" %d", pcr->pr_groups[i]);
	(void)printf("\n");
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid ...\n", __progname);
	exit(EX_USAGE);
}
