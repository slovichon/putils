/* $Id$ */

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#include <err.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "putils.h"
#include "util.h"

struct plink {
	struct ptnode	*pl_ptn;
	struct plink	*pl_next;
};

struct ptnode {
	pid_t		 ptn_pid;
	pid_t		 ptn_ppid;
	char 		*ptn_cmd;
	struct plink	 ptn_children;
};

static		char  *join(char **);
static		void   buildtree(struct ptnode **);
static		void   doproc(struct ptnode *, char *);
static		void   findtree(struct ptnode *, pid_t, int);
static		void   freetree(struct ptnode *);
struct		plink *findpl(struct plink *, pid_t);
static __dead	void   usage(void);

static int		showinit = 0;
static int		level = 0;

int
main(int argc, char *argv[])
{
	struct ptnode *root;
	int ch;

	while ((ch = getopt(argc, argv, "a")) != -1) {
		switch (ch) {
		case 'a':
			showinit = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argv += optind;

	buildtree(&root);
	if (argc)
		while (*argv)
			doproc(root, *argv++);
	else
		doproc(root, "1");
	freetree(root);
	exit(EXIT_SUCCESS);
}

static void
buildtree(struct ptnode **root)
{
	struct plink unacc, rootpl, *pl, *child;
	char **argv, buf[_POSIX2_LINE_MAX];
	struct kinfo_proc2 *kip;
	int i, pcnt;
	kvm_t *kd;

	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL, (char *)NULL,
	     KVM_NO_FILES, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	kip = kvm_getproc2(kd, KERN_PROC_KTHREAD, 0, sizeof(*kip), &pcnt);
	if (kip == NULL || pcnt == 0)
		errx(EX_OSERR, "kvm_getprocs: %s", kvm_geterr(kd));
	unacc.pl_next = NULL;
	for (i = 0; i < pcnt; i++, kip++) {
		if ((child = malloc(sizeof(*child))) == NULL)
			err(EX_OSERR, "malloc");
		if ((child->pl_ptn = malloc(sizeof(*child->pl_ptn))) == NULL)
			err(EX_OSERR, "malloc");
		if ((pl = findpl(&unacc, kip->p_ppid)) == NULL) {
			/* No parent; add to unaccounted list. */
			child->pl_ptn->ptn_children.pl_next = NULL;
			child->pl_next = unacc.pl_next;
			unacc.pl_next = child;
		} else {
			/* Found parent; add there. */
			child->pl_next = pl->pl_ptn->ptn_children.pl_next;
			pl->pl_ptn->ptn_children.pl_next = child;
		}
		if ((argv = kvm_getargv2(kd, kip, 0)) == NULL)
			child->pl_ptn->ptn_cmd = strdup(kip->p_comm);
			/* err(EX_OSERR, "kvm_getargv: %s", kvm_geterr(kd)); */
		else
			child->pl_ptn->ptn_cmd = join(argv);
		child->pl_ptn->ptn_pid = kip->p_pid;
		child->pl_ptn->ptn_ppid = kip->p_ppid;
	}
	(void)kvm_close(kd);

	/* Find and remove root node. */
	if ((pl = findpl(&unacc, 0)) == NULL)
		errx(EX_SOFTWARE, "improper tree building");
	rootpl.pl_next = pl->pl_next;
	pl->pl_next = rootpl.pl_next->pl_next;
	*root = rootpl.pl_next->pl_ptn;

	/* Rescan unaccounted-for process nodes. */
	while (unacc.pl_next != NULL) {
		/* Remove child. */
		child = unacc.pl_next;
		unacc.pl_next = child->pl_next;
		/* Scan both unaccounted-for nodes and nodes under root. */
		if ((pl = findpl(&unacc, child->pl_ptn->ptn_ppid)) == NULL)
			if ((pl = findpl(&rootpl, child->pl_ptn->ptn_ppid)) ==
			    NULL)
				errx(EX_SOFTWARE, "improper tree building (%d)",
				    child->pl_ptn->ptn_pid);
		child->pl_next = pl->pl_next->pl_ptn->ptn_children.pl_next;
		pl->pl_next->pl_ptn->ptn_children.pl_next = child;
	}
}

struct plink *
findpl(struct plink *pl, pid_t pid)
{
	struct plink *plchild;

	for (; pl->pl_next != NULL; pl = pl->pl_next) {
		if (pl->pl_next->pl_ptn->ptn_pid == pid)
			return (pl);
		/* Recursively search grandchildren. */
		if ((plchild = findpl(&pl->pl_next->pl_ptn->ptn_children,
		     pid)) != NULL)
			return (plchild);
	}
	return (NULL);
}

char *
join(char **s)
{
	size_t siz = 0;
	char *p, **t;

	for (t = s; *t != NULL; t++)
		/*
		 * Add 1+strlen for each, which takes care
		 * of NUL for the last argument.
		 */
		siz += strlen(*t) + 1;

	if ((p = malloc(siz)) == NULL)
		err(EX_OSERR, "malloc");
	p[0] = '\0';
	for (t = s; *t != NULL; t++) {
		strlcat(p, *t, siz);
		strlcat(p, " ", siz);
	}
	return (p);
}

static void
freetree(struct ptnode *ptn)
{
	struct plink *pl, *next;

	free(ptn->ptn_cmd);
	for (pl = ptn->ptn_children.pl_next; pl != NULL; pl = next) {
		next = pl->pl_next;
		freetree(pl->pl_ptn);
		free(pl);
	}
	free(ptn);
}

static void
doproc(struct ptnode *root, char *s)
{
	pid_t pid;

	if (!parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}
	findtree(root, pid, 0);
}

static void
findtree(struct ptnode *root, pid_t pid, int found)
{
	struct plink *pl;

	if (root->ptn_pid == pid)
		found = 1;
	if (found) {
		(void)printf("%*s%d %s\n", level * 2, "", root->ptn_pid,
		    root->ptn_cmd);
		level++;
	}
	for (pl = root->ptn_children.pl_next; pl != NULL;
	     pl = pl->pl_next)
		findtree(pl->pl_ptn, pid, found);
	if (found)
		level--;
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-a] [pid|user ...]\n", __progname);
	exit(EX_USAGE);
}
