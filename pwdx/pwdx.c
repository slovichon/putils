/* $Id$ */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/queue.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "putils.h"
#include "util.h"

struct cnp {
	char *c_nam;
	SLIST_ENTRY(cnp) next;
};

static		void doproc(char *);
static __dead	void usage(void);

static int force = 0;
static SLIST_HEAD(, cnp) cnph;

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

static void
doproc(char *s)
{
	char *path, cwd[MAXPATHLEN];
	struct stat st, pst;
	struct cnp *pcnp, *next;
	pid_t pid;

	if ((path = getpidpath(s, &pid, 0)) == NULL) {
		xwarn("cannot examine %s", s);
		return;
	}
	(void)snprintf(cwd, sizeof(cwd), "%s/cmd", path);
	free(path);
	if (stat(cwd, &st) == -1) {
		warnx("cannot examine %s", s);
		return;
	}
	for (;;) {
		(void)snprintf(cwd, sizeof(cwd), "%s/..", cwd);
		if (stat(cwd, &pst) == -1)
			warnx("cannot examine %s", s);
		if (pst.st_ino == st.st_ino)
			break;
		if ((pcnp = malloc(sizeof(*pcnp))) == NULL)
			err(EX_OSERR, "malloc");
		SLIST_INSERT_HEAD(&cnph, pcnp, next);
		(void)memcpy(&st, &pst, sizeof(st));

	}
	(void)printf("%d:\t", pid);
	for (pcnp = SLIST_FIRST(&cnph); pcnp != NULL; pcnp = next) {
		next = SLIST_NEXT(pcnp, next);
		(void)printf("/%s", pcnp->c_nam);
		free(pcnp);
	}
	(void)printf("\n");
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-F] pid ...\n", __progname);
	exit(EX_USAGE);
}
