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

#include "putils.h"
#include "util.h"

static void doproc(char *);
static void usage(void) __attribute__((__noreturn__));

static kvm_t *kd = NULL;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];

	if (argc < 2)
		usage();
	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL,
	     (char *)NULL, O_RDONLY, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	while (*++argv != NULL)
		doproc(*argv);
	(void)kvm_close(kd);
	exit(EXIT_SUCCESS);
}

static void
doproc(char *s)
{
	struct vm_map_entry vme_p;
	struct vmspace vmspace;
	struct kinfo_proc2 *kip;
	char **argv, *p;
	pid_t pid;
	int pcnt;

	if (!parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}
	kip = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kip), &pcnt);
	if (kip == NULL)
		warn("kvm_getproc2: %s", kvm_geterr(kd));
	else if (pcnt == 0) {
		errno = ESRCH;
		xwarn("cannot examine %s", s);
	} else {
		if ((argv = kvm_getargv2(kd, kip, 0)) == NULL)
			(void)printf("%d:\t%s\n", pid, kip->p_comm);
		else {
			p = join(argv);
			(void)printf("%d:\t%s\n", pid, p);
			free(p);
		}
		if ((kvm_read(kd, (u_long)kip->p_vmspace, &vmspace,
		     sizeof(vmspace))) != sizeof(vmspace)) {
			warnx("kvm_read: %s", kvm_geterr(kd));
			return;
		}
		vme_p.next = vmspace.vm_map.header.next;
		do {
			if ((kvm_read(kd, (u_long)vme_p.next, &vme_p,
			     sizeof(vme_p))) != sizeof(vme_p)) {
				warnx("kvm_read: %s", kvm_geterr(kd));
				return;
			}
//			if (0)
				(void)printf("%lx\n", vme_p.start);
		} while (vme_p.next != vmspace.vm_map.header.next);
	}
}

static void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid|core ...", __progname);
	exit(EX_USAGE);
}
