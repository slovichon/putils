/* $Id$ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/queue.h>
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

#define PID_MAX INT_MAX
#define NOPROC (-1)

struct proc_entry {
	pid_t  pe_pid;
	uid_t  pe_ruid;
	uid_t  pe_euid;
	uid_t  pe_svuid;
	gid_t  pe_rgid;
	gid_t  pe_egid;
	gid_t  pe_svgid;
	short  pe_ngroups;
	gid_t *pe_groups;
};

kvm_t *kd = NULL;

static		void fromcore(int fd);
static		void fromelf(int fd);
static		void fromkvm(pid_t);

static		void doproc(char *);
static		void printproc(struct proc_entry *);
static __dead	void usage(void);

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
	struct proc_entry pe;
	struct kinfo_proc2 *kip;
	char buf[_POSIX2_LINE_MAX];
	int nproc;
	if (kd == NULL) {
		kd = kvm_openfiles(NULL, NULL, NULL, KVM_NO_FILES, buf);
		if (kd == NULL)
			errx(EX_OSERR, "kvm_openfiles: %s", buf);
	}

	kip = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kip), &nproc);
	if (kip == NULL)
		warnx("kvm_getproc2: %s", kvm_geterr(kd));
	else if (nproc == 0) {
		errno = ENOENT;
		warn("cannot examine %d", pid);
	} else {
		size_t siz;

		(void)memset(&pe, 0, sizeof(pe));
		siz = kip->p_ngroups * sizeof(*pe.pe_groups);
		if ((pe.pe_groups = malloc(siz)) == NULL) {
			warn(NULL);
			return;
		}
		(void)memcpy(pe.pe_groups, kip->p_groups, siz);
		pe.pe_pid	= pid;
		pe.pe_ruid	= kip->p_ruid;
		pe.pe_euid	= kip->p_uid;
		pe.pe_svuid	= kip->p_svuid;
		pe.pe_rgid	= kip->p_rgid;
		pe.pe_egid	= kip->p_gid;
		pe.pe_svgid	= kip->p_svgid;
		pe.pe_ngroups	= kip->p_ngroups;
		printproc(&pe);
		free(pe.pe_groups);
	}
}

static void
printproc(struct proc_entry *pe)
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

	(void)printf("%d:\t", pe->pe_pid);
	if (pe->pe_euid == pe->pe_ruid)
		(void)printf("e/");
	else
		(void)printf("euid=%d ", pe->pe_euid);
	if (pe->pe_ruid == pe->pe_svuid)
		(void)printf("r/");
	else
		(void)printf("ruid=%d ", pe->pe_ruid);
	(void)printf("suid=%d  ", pe->pe_svuid);
	if (pe->pe_egid == pe->pe_rgid)
		(void)printf("e/");
	else
		(void)printf("egid=%d ", pe->pe_egid);
	if (pe->pe_rgid == pe->pe_svgid)
		(void)printf("r/");
	else
		(void)printf("rgid=%d ", pe->pe_rgid);
	(void)printf("sgid=%d\n", pe->pe_svgid);
	(void)printf("\tgroups:");
	for (i = 0; i < pe->pe_ngroups; i++)
		(void)printf(" %d", pe->pe_groups[i]);
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
	unsigned long l;
	int isnum, fd, ch;
	char *t, fil[MAXPATHLEN];
	struct statfs fst;
	FILE *fp;

	isnum = 1;
	for (t = s; *t != '\0'; t++)
		if (!isdigit(*t)) {
			isnum = 0;
			break;
		}

	if (isnum) {
		l = strtoul(s, NULL, 10);
		if (l <= PID_MAX)
			fromkvm((pid_t)l);
		return;
	}

	/* Try to read a core. */
	if ((fd = open(s, O_RDONLY)) == -1) {
		if (errno != ENOENT)
			goto bail;
	} else {
		fromcore(fd);
		(void)close(fd);
	}

	/* Try /proc/<pid>/status */
	if (statfs(s, &fst) == -1)
		goto bail;
	if (strcmp(fst.f_fstypename, MOUNT_PROCFS) != 0)
		goto bail;
	(void)snprintf(fil, sizeof(fil), "%s/status", s);
	if ((fp = fopen(fil, "r")) == NULL)
		goto bail;
	/* This worked, use pid from pid field. */
	while (!isdigit(ch = fgetc(fp)) && ch != EOF)
		;
	l = 0;
	isnum = 0;
	while (isdigit(ch)) {
		isnum = 1;
		l = l * 10 + (ch - '0');
		ch = fgetc(fp);
	}
	(void)fclose(fp);

	if (isnum && l <= PID_MAX)
		fromkvm((pid_t)l);
	return;

bail:
	warn("cannot examine %s", s);
	return;
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [pid | core]\n", __progname);
	exit(1);
}
