/* $Id$ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/mount.h>

#include <ctype.h>
#include <elf_abi.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include "putils.h"
#include "util.h"

struct prcred {
	pid_t  pr_pid;
	uid_t  pr_ruid;
	uid_t  pr_euid;
	uid_t  pr_svuid;
	gid_t  pr_rgid;
	gid_t  pr_egid;
	gid_t  pr_svgid;
	short  pr_ngroups;
	gid_t *pr_groups;
};

static void doproc(char *);
static void fromcore(int fd);
static void fromelf(int fd);
static void fromkvm(pid_t);
static void printproc(struct prcred *);
static void usage(void) __attribute__((__noreturn__));

static kvm_t *kd = NULL;

int
main(int argc, char *argv[])
{

	if (argc < 2)
		usage();
	while (*++argv != NULL)
		doproc(*argv);
	if (kd != NULL)
		(void)kvm_close(kd);
	exit(0);
}

static void
fromkvm(pid_t pid)
{
	char buf[_POSIX2_LINE_MAX];
	struct kinfo_proc2 *kip;
	struct prcred prc;
	int nproc;

	if (kd == NULL)
		if ((kd = kvm_openfiles((char *)NULL, (char *)NULL, (char *)NULL,
		     KVM_NO_FILES, buf)) == NULL)
			errx(EX_OSERR, "kvm_openfiles: %s", buf);

	kip = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kip), &nproc);
	if (kip == NULL)
		warnx("kvm_getproc2: %s", kvm_geterr(kd));
	else if (nproc == 0) {
		errno = ENOENT;
		warn("cannot examine %d", pid);
	} else {
		size_t siz;

		(void)memset(&prc, 0, sizeof(prc));
		siz = kip->p_ngroups * sizeof(*prc.pr_groups);
		if ((prc.pr_groups = malloc(siz)) == NULL) {
			warn(NULL);
			return;
		}
		(void)memcpy(prc.pr_groups, kip->p_groups, siz);
		prc.pr_pid	= pid;
		prc.pr_ruid	= kip->p_ruid;
		prc.pr_euid	= kip->p_uid;
		prc.pr_svuid	= kip->p_svuid;
		prc.pr_rgid	= kip->p_rgid;
		prc.pr_egid	= kip->p_gid;
		prc.pr_svgid	= kip->p_svgid;
		prc.pr_ngroups	= kip->p_ngroups;
		printproc(&prc);
		free(prc.pr_groups);
	}
}

static void
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

static void
fromcore(int fd)
{

}

static void
fromelf(int fd)
{

}

static void
doproc(char *s)
{
	pid_t pid;

	if (parsepid(s, &pid)) {
		fromkvm(pid);
	} else if (0 /* corefile */) {
	} else {
		xwarn("cannot examine %s", s);
		return;
	}
}

static void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid|core\n", __progname);
	exit(EX_USAGE);
}
