/* $Id$ */

#include <sys/param.h>
#include <sys/vnode.h>

#include <kvm.h>
#include <stdio.h>
#include <sysexits.h>
#include <err.h>
#include <stdlib.h>

#include "putils.h"

#define _PATH_PROC

int force = 0;

int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "F")) == -1) {
		switch (ch) {
		case 'F':
			force = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argv += optind;

	while (*argv != NULL)
		doproc(*argv);
	exit(EXIT_SUCCESS);
}

struct cnp {
	char *c_nam;
	SLIST_ENTRY(struct cnp) next;
};

static SLIST_HEAD(, struct cnp) cnph;

static void
doproc(char *s)
{
	struct kinfo_proc *kip;
	char cwd[MAXPATHLEN];
	pid_t pid;
	int pcnt;
	struct stat st, pst;
	struct cnp *pcnp, *next;

	if (!getpidpath(s, cwd)) {
		warnx("cannot examine %s", s);
		continue;
	}
	(void)snprintf(cwd, sizeof(cwd), "%s/cmd", cwd);
	if (stat(cwd, &st) == -1)
		/* xxx */
	for (;;) {
		(void)snprintf(cwd, sizeof(cwd), "%s/..", cwd);
		if (stat(cwd, &pst) == -1)
			/* xxx */
		if (pst.st_ino == st.st_ino)
			break;
		if ((pcnp = malloc(sizeof(*pcnp))) == NULL)
			err(EX_OSERR, "malloc");
		SLIST_INSERT_HEAD(&cnph, pcnp, next);
		(void)memcpy(&st, &pst, sizeof(st));
		
	}
	(void)printf("%d:\t", pid);
	for (pcnp = SLIST_HEAD(cnph); pcnp != NULL; pcnp = next) {
		next = pcnp->next;
		(void)printf("/%s", pcnp->c_nam);
		free(pcnp);
	}
	(void)printf("\n");
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-F] pid ...");
	exit(EX_USAGE);
}
