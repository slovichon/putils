/* $Id$ */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <dev/systrace.h>

#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#define PID_MAX		INT_MAX
#define _PATH_SYSTRACE	"/dev/systrace"
#define NSYSCALLS	512

struct cnp {
	char			*c_name;
	SLIST_ENTRY(cnp)	 c_next;
};

int				 pwdx(const char *);
__dead void			 usage(void);

int				 cfd;
SLIST_HEAD(, cnp)		 cwd;

int
main(int argc, char *argv[])
{
	int fd, c, status;

	while ((c = getopt(argc, argv, "")) != -1)
		switch (c) {
		default:
			usage();
			/* NOTREACHED */
		}
	argv += optind;
	if (*argv == NULL)
		usage();
	if ((fd = open(_PATH_SYSTRACE, O_RDONLY)) == -1)
		err(EX_OSERR, "%s", _PATH_SYSTRACE);
	if (ioctl(fd, STRIOCCLONE, &cfd) == -1)
		err(EX_OSERR, "STRIOCCLONE");
	(void)close(fd);
	status = 0;
	while (*argv != NULL)
		status |= pwdx(*argv++);
	(void)close(cfd);
	exit(status ? EX_UNAVAILABLE : EX_OK);
}

pid_t curpid;

void
sighandler(int signo)
{
	if (curpid)
		if (ioctl(cfd, STRIOCDETACH, &curpid) == -1)
			err(EX_OSERR, "STRIOCDETACH");
	exit(0);
}

int
pwdx(const char *s)
{
	struct sigaction newact, oldact;
	struct systrace_policy strpol;
	struct str_message strmsg;
	struct stat st, rootst;
	struct cnp *cnp, *ncnp;
	struct dirent *d_ent;
	const char *errstr;
	int i, error;
	pid_t pid;
	ssize_t n;
	DIR *dp;

	pid = strtonum(s, 0, PID_MAX, &errstr);
	if (errstr != NULL) {
		warnx("%s: %s", s, errstr);
		return (1);
	}
	curpid = pid;
	if (sigaction(SIGINT, NULL, &newact) == -1)
		err(EX_OSERR, "sigaction");
	newact.sa_handler = sighandler;
	if (sigaction(SIGINT, &newact, &oldact) == -1)
		err(EX_OSERR, "sigaction");
	if (ioctl(cfd, STRIOCATTACH, &pid) == -1) {
		if (errno == EPERM) {
			warn("%s", s);
			return (1);
		}
		err(EX_OSERR, "STRIOCATTACH");
	}
	strpol.strp_op = SYSTR_POLICY_NEW;
	strpol.strp_maxents = NSYSCALLS;
	if ((error = ioctl(cfd, STRIOCPOLICY, &strpol)) == -1)
		goto fail;
	strpol.strp_op = SYSTR_POLICY_ASSIGN;
	strpol.strp_pid = pid;
	if ((error = ioctl(cfd, STRIOCPOLICY, &strpol)) == -1)
		goto fail;
	for (i = 0; i < NSYSCALLS; i++) {
		strpol.strp_op = SYSTR_POLICY_MODIFY;
		strpol.strp_code = i;
		strpol.strp_policy = SYSTR_POLICY_ASK;
		if ((error = ioctl(cfd, STRIOCPOLICY, &strpol)) == -1)
			goto fail;
	}
	while ((n = read(cfd, &strmsg, sizeof(strmsg))) ==
	    sizeof(strmsg))
		if (strmsg.msg_type == SYSTR_MSG_ASK)
			break;
	if (n == -1) {
		error = -1;
		goto fail;
	}
	error = ioctl(cfd, STRIOCGETCWD, &pid);
fail:
	if (ioctl(cfd, STRIOCDETACH, &pid) == -1)
		err(EX_OSERR, "STRIOCDETACH");
	curpid = 0;
	if (sigaction(SIGINT, &oldact, NULL) == -1)
		err(EX_OSERR, "sigaction");
	if (error == -1) {
		warn("STRIOCPOLICY %s", s);
		return (1);
	}
	error = 0;
	if (stat("/", &rootst) == -1)
		err(EX_OSERR, "stat /");
	SLIST_INIT(&cwd);
	for (;;) {
		if (stat(".", &st) == -1)
			err(EX_OSERR, "stat .");
		if (st.st_ino == rootst.st_ino)
			break;
		if ((dp = opendir("..")) == NULL)
			err(EX_OSERR, "stat ..");
		while ((d_ent = readdir(dp)) != NULL)
			if (d_ent->d_fileno == st.st_ino) {
				if ((cnp = malloc(sizeof(*cnp))) == NULL)
					err(EX_OSERR, "malloc");
				(void)memset(cnp, 0, sizeof(*cnp));
				if ((cnp->c_name =
				    strdup(d_ent->d_name)) == NULL)
					err(EX_OSERR, "strdup");
				SLIST_INSERT_HEAD(&cwd, cnp, c_next);
				break;
			}
		if (d_ent == NULL) {
			warnx("cannot find working directory for %s", s);
			error = 1;
			goto done;
		}
		(void)closedir(dp);
		if (chdir("..") == -1) {
			warnx("chdir ..");
			error = 1;
			goto done;
		}
	}
done:
	(void)printf("/");
	for (cnp = SLIST_FIRST(&cwd); cnp != NULL; cnp = ncnp) {
		ncnp = SLIST_NEXT(cnp, c_next);
		if (!error)
			(void)printf("%s%s", cnp->c_name,
			    ncnp == NULL ? "" : "/");
		free(cnp->c_name);
		free(cnp);
	}
	(void)printf("\n");
	return (error);
}

void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s pid ...\n", __progname);
	exit(EX_USAGE);
}
