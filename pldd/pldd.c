/* $Id$ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_map.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#define PID_MAX	INT_MAX

int		pldd(char *);
__dead void	usage(void);

kvm_t		*kd = NULL;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];
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
	if ((kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY,
	    buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	status = 0;
	while (*argv != NULL)
		status |= pldd(*argv++);
	(void)kvm_close(kd);
	exit(EXIT_SUCCESS);
}

int
pldd(char *s)
{
	struct vm_map_entry vme_p;
	struct vmspace vmspace;
	struct kinfo_proc2 *kp;
	const char *errstr;
	char **argv;
	pid_t pid;
	int pcnt;

	pid = strtonum(s, 0, PID_MAX, &errstr);
	if (errstr != NULL) {
		warnx("%s: %s", s, errstr);
		return (1);
	}
	if ((kp = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kp),
	    &pcnt)) == NULL)
		errx(EX_OSERR, "kvm_getproc2: %s", kvm_geterr(kd));
	if (pcnt == 0) {
		errno = ESRCH;
		warn("%s", s);
		return (1);
	}
	if ((argv = kvm_getargv2(kd, kp, 0)) == NULL)
		(void)printf("%d:\t%s\n", pid, kp->p_comm);
	else {
		(void)printf("%d:\t", pid);
		for (; *argv != NULL; argv++)
			(void)printf("%s%s", *argv, argv[1] == NULL ?
			    "" : " ");
		(void)printf("\n");
	}
	if ((kvm_read(kd, (u_long)kp->p_vmspace, &vmspace,
	     sizeof(vmspace))) != sizeof(vmspace))
		errx(EX_OSERR, "kvm_read: %s", kvm_geterr(kd));
	vme_p.next = vmspace.vm_map.header.next;
	do {
		if ((kvm_read(kd, (u_long)vme_p.next, &vme_p,
		     sizeof(vme_p))) != sizeof(vme_p))
			errx(EX_OSERR, "kvm_read: %s", kvm_geterr(kd));
//		if (0)
			(void)printf("%lx\n", vme_p.start);
	} while (vme_p.next != vmspace.vm_map.header.next);
	return (0);
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid ...", __progname);
	exit(EX_USAGE);
}
