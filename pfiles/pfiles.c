/* $Id$ */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/filedesc.h>
#include <sys/resource.h>
#include <sys/stat.h>

#define _KERNEL /* XXX */
#include <sys/file.h>
#undef _KERNEL

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

static		void doproc(char *);
static __dead	void usage(void);

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
	int pcnt, saw, i, j, pr_rdwr;
	char **argv, *p, *fileflags;
	struct file fil, *fp, **fpp;
	struct kinfo_proc2 *kip;
	struct filedesc fdt;
	struct plimit pl;
	size_t siz;
	pid_t pid;

	if (!parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}
	kip = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kip), &pcnt);
	if (kip == NULL)
		warnx("kvm_getproc2: %s", kvm_geterr(kd));
	else if (pcnt == 0) {
		errno = ESRCH;
		warn("cannot examine: %s", s);
	} else {
		if (kvm_read(kd, kip->p_fd, &fdt, sizeof(fdt)) != sizeof(fdt)) {
			warn("kvm_read: %s", kvm_geterr(kd));
			return;
		}
		if (kvm_read(kd, kip->p_limit, &pl, sizeof(pl)) != sizeof(pl)) {
			warn("kvm_read: %s", kvm_geterr(kd));
			return;
		}
		siz = sizeof(char) * fdt.fd_lastfile;
		if ((fileflags = malloc(siz)) == NULL)
			err(EX_OSERR, NULL);
		if (kvm_read(kd, (u_long)fdt.fd_ofileflags, fileflags, siz) !=
		    siz) {
			warn("kvm_read: %s", kvm_geterr(kd));
			return;
		}
		if ((argv = kvm_getargv2(kd, kip, 0)) == NULL)
			(void)printf("%d:\t%s\n", pid, kip->p_comm);
		else {
			p = join(argv);
			(void)printf("%d:\t%s\n", pid, p);
			free(p);
		}
		(void)printf("  Current rlimit: %lld file descriptors\n",
		    pl.pl_rlimit[RLIMIT_NOFILE].rlim_cur);
		for (j = 0, fpp = fdt.fd_ofiles;
		     j < fdt.fd_lastfile;
		     fpp++, j++) {
			if (kvm_read(kd, (u_long)fpp, &fp, sizeof(fp)) !=
			    sizeof(fp))
				err(EX_OSERR, "kvm_read: %s", kvm_geterr(kd));
			if (fp == NULL)
				continue;
			if (kvm_read(kd, (u_long)fp, &fil, sizeof(fil)) !=
			    sizeof(fil))
				err(EX_OSERR, "kvm_read: %s", kvm_geterr(kd));
			switch (fil.f_type) {
			case S_IFIFO:	p = "S_IFIFO";  break;
			case S_IFCHR:	p = "S_IFCHR";  break;
			case S_IFDIR:	p = "S_IFDIR";  break;
			case S_IFBLK:	p = "S_IFBLK";  break;
			case S_IFREG:	p = "S_IFREG";  break;
			case S_IFLNK:	p = "S_IFLNK";  break;
			case S_IFSOCK:	p = "S_IFSOCK"; break;
			case S_IFWHT:	p = "S_IFWHT";  break;
			default:	p = "???";	break;
			}
			(void)printf("%4d: %s ", j, p);
			(void)printf("mode: 0%03o ", 0);
			(void)printf("dev: %d,%d ", 0,0);
			(void)printf("ino: %d ", 0);
			(void)printf("uid: %d ", 0);
			(void)printf("gid: %d ", 0);
			(void)printf("rdev: %d,%d\n	", 0,0);
			saw = 0;
			pr_rdwr = 0;
			for (i = 0; fil.f_flag >> i; i++) {
				switch (fil.f_flag & (1 << i)) {
				case FREAD:
				case FWRITE:
					if (pr_rdwr) {
						p = NULL;
						break;
					}
					pr_rdwr = 1;
					if (fil.f_flag & (FREAD | FWRITE))
						p = "O_RDWR";
					else if (fil.f_flag & FREAD)
						p = "O_RDONLY";
					else if (fil.f_flag & FWRITE)
						p = "O_WRONLY";
					break;
				case FNONBLOCK:	p = "O_NONBLOCK"; break;
				case FAPPEND:	p = "O_APPEND";	break;
				case FHASLOCK:
					p = "O_SHLOCK";
					p = "O_EXLOCK";
					break;
				case FASYNC:	p = "O_ASYNC";	break;
				case FFSYNC:	p = "O_SYNC";	break;
				default:	p = NULL;	break;
				}
				if (p == NULL)
					saw = 0;
				else {
					(void)printf("%s%s", saw ? "|" : "", p);
					saw = 1;
				}
			}
			if (fileflags[j])
				(void)printf(" ");
			saw = 0;
			for (i = 0; fileflags[j] >> i; i++) {
				switch (fileflags[j] & (1 << i)) {
				case UF_EXCLOSE: p = "FD_CLOEXEC"; break;
				default: p = NULL; break;
				}
				if (p == NULL)
					saw = 0;
				else {
					(void)printf("%s%s", saw ? "|" : "", p);
					saw = 1;
				}
			}
			(void)printf("\n");
		}
		free(fileflags);
	}
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid\n", __progname);
	exit(EX_USAGE);
}
