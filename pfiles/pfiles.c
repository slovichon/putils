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

#define PID_MAX	INT_MAX

int		 pfiles(char *);
__dead void	 usage(void);

kvm_t		*kd = NULL;
int		 prnames = 0;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];
	int c, status;

	while ((c = getopt(argc, argv, "n")) != -1)
		switch (c) {
		case 'n':
			prnames = 1;
			break;
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
		status |= pfiles(*argv++);
	(void)kvm_close(kd);
	exit(status ? EX_UNAVAILABLE : EX_OK);
}

int
pfiles(char *s)
{
	char **argv, *p, *fileflags;
	int pcnt, saw, i, j, pr_rdwr;
	struct file fil, *fp, **fpp;
	struct kinfo_proc2 *kp;
	struct filedesc fdt;
	const char *errstr;
	struct plimit pl;
	size_t siz;
	int error;
	pid_t pid;

	error = 0;
	pid = strtonum(s, 0, PID_MAX, &errstr);
	if (errstr != NULL) {
		warn("%s: %s", s, errstr);
		return (1);
	}
	if ((kp = kvm_getproc2(kd, KERN_PROC_PID, pid, sizeof(*kp), &pcnt)) == NULL)
		errx(EX_OSERR, "kvm_getproc2: %s", kvm_geterr(kd));
	if (pcnt == 0) {
		errno = ESRCH;
		warn("%s", s);
		return (1);
	}
	if (kvm_read(kd, kp->p_fd, &fdt, sizeof(fdt)) != sizeof(fdt)) {
		warn("kvm_read: %s", kvm_geterr(kd));
		return (1);
	}
	if (kvm_read(kd, kp->p_limit, &pl, sizeof(pl)) != sizeof(pl)) {
		warn("kvm_read: %s", kvm_geterr(kd));
		return (1);
	}
	siz = sizeof(char) * fdt.fd_lastfile;
	if ((fileflags = malloc(siz)) == NULL)
		err(EX_OSERR, NULL);
	if (kvm_read(kd, (u_long)fdt.fd_ofileflags, fileflags, siz) !=
	    (ssize_t)siz) {
		warn("kvm_read: %s", kvm_geterr(kd));
		goto done;
	}
	if ((argv = kvm_getargv2(kd, kp, 0)) == NULL)
		(void)printf("%d:\t%s\n", pid, kp->p_comm);
	else {
		(void)printf("%d:\t", pid);
		for (; *argv != NULL; argv++)
			(void)printf("%s%s", *argv,
			    argv[1] == NULL ? "" : " ");
		(void)printf("\n");
	}
	(void)printf("  Current rlimit: %lld file descriptors\n",
	    pl.pl_rlimit[RLIMIT_NOFILE].rlim_cur);
	for (j = 0, fpp = fdt.fd_ofiles; j < fdt.fd_lastfile; fpp++,
	    j++) {
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
			case UF_EXCLOSE:
				p = "FD_CLOEXEC";
				break;
			default:
				p = NULL;
				break;
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
done:
	free(fileflags);
	return (error);
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid\n", __progname);
	exit(EX_USAGE);
}
