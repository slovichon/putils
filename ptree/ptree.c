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

#define hash(pid, max) (((pid * pid)^ -1) % max)

struct plink {
	struct ptnode	*pl_ptn;
	struct plink	*pl_next;
	int		 pl_last;
} **phashtab;

struct ptnode {
	pid_t		 ptn_pid;
	pid_t		 ptn_ppid;
	pid_t		 ptn_pgid;
	char 		*ptn_cmd;
	struct ptnode	*ptn_parent;
	struct plink	 ptn_children;
};

static		void   buildtree(struct ptnode **);
static		void   doproc(struct ptnode *, char *);
static		void   findtree(struct ptnode *, pid_t, int);
static		void   freetree(struct ptnode *);
struct		plink *findpl(struct plink *, pid_t);
static __dead	void   usage(void);

static int	showinit = 0;
static int	level = 0;
static size_t	phashtabsiz = 0;

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
	argc -= optind;

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
	size_t pos;
	kvm_t *kd;

	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL, (char *)NULL,
	     KVM_NO_FILES, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	kip = kvm_getproc2(kd, KERN_PROC_KTHREAD, 0, sizeof(*kip), &pcnt);
	if (kip == NULL || pcnt == 0)
		errx(EX_OSERR, "kvm_getprocs: %s", kvm_geterr(kd));
	unacc.pl_next = NULL;
	phashtabsiz = pcnt * 2;
	if ((phashtab = calloc(phashtabsiz, sizeof(*phashtab))) == NULL)
		err(EX_OSERR, "calloc");
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
		child->pl_ptn->ptn_pgid = kip->p_pgid;
		/* Place in hash queue. */
		if ((pl = malloc(sizeof(*pl))) == NULL)
			err(EX_OSERR, "malloc");
		pos = hash(kip->p_pid, pcnt * 2);
		pl->pl_next = phashtab[pos];
		phashtab[pos] = pl;
		pl->pl_ptn = child->pl_ptn;
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

static void
freetree(struct ptnode *ptn)
{
	struct plink ph, *pd, *pl, *next, *pt;
	int i;

	for (i = 0; i < phashtabsiz; i++)
		for (pl = phashtab[i]; pl != NULL; pl = next) {
			next = pl->pl_next;
			free(pl);
		}
	free(phashtab);

	/* Yes, this is silly. */
	if ((pd = malloc(sizeof(*pd))) == NULL)
		err(EX_OSERR, "malloc");

	ph.pl_next = pd;
	pd->pl_ptn = ptn;
	pd->pl_next = NULL;

	while (ph.pl_next != NULL) {
		pl = ph.pl_next;
		ptn = pl->pl_ptn;
		if (ptn->ptn_children.pl_next != NULL) {
			/* Find tail. */
			for (pt = &ptn->ptn_children;
			     pt->pl_next != NULL;
			     pt = pt->pl_next)
				;
			/* Insert into list to remove. */
			pt->pl_next = pl->pl_next;
			pl->pl_next = ptn->ptn_children.pl_next;
		}
		ph.pl_next = pl->pl_next;
		free(ptn->ptn_cmd);
		free(ptn);
		free(pl);
	}
}

static struct plink *
pldup(struct plink *pl)
{
	struct plink *dup;

	if ((dup = malloc(sizeof(*dup))) == NULL)
		err(EX_OSERRR, NULL);
	dup->pl_next = NULL;
	dup->pl_ptn = pl->pl_ptn;
	return (dup);
}

static void
doproc(struct ptnode *root, char *s)
{
	struct plink *anc, *desc, *pl, *target, *next, *dup;
	pid_t pid;

	if (!parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}
	findtree(root, pid, 0);

	/* Find process. */
	if (root->ptn_pid == pid)
		found = 1;
	for (pl = root->ptn_children.pl_next; pl != NULL;
	     pl = pl->pl_next)
		findtree(pl->pl_ptn, pid, found);
	for (pl = root; pl != NULL; pl = pl->pl_next) {
		if (pl->pl_ptn->ptn_pid == pid)
			break;
		for (next = pl->pl_ptn->ptn_children.pl_next;
		     next != NULL; next = next->pl_next) {
			if ((dup = malloc(sizeof(*dup))) == NULL)
				err(EX_OSERR, NULL);
			dup->pl_ptn = next->pl_ptn;
			dup->pl_next = pl->pl_next;
			pl->pl_next = dup;
		}
	}
	if (pl == NULL) {
		errno = ENOENT;
		warn("cannot examine %d", pid);
		return;
	}

	/* Collect and dump ancestors. */
	anc = NULL;
	while (showinit ? ptn->ptn_pid != 0 :
	    ptn->ptn_pgid == ptn->ptn_pid) {
		if ((pl = malloc(sizeof(*pl))) == NULL)
			err(EX_OSERR, NULL);
		pl->pl_next = anc;
		pl->pl_ptn = target;
		anc = pl;
		target = target->ptn_parent;
	}

	level = 0;
	for (level = 0; anc != NULL; anc = next, level++) {
		(void)printf("%*s%d %s\n", level * 2, "", anc->ptn_pid,
		    anc->ptn_cmd);
		next = anc->pl_next;
		free(anc);
	}

	/* Collect and dump descendents. */
	desc = ;
	for (; desc != NULL; desc = next) {
		(void)printf("%*s%d %s\n", level * 2, "", desc->ptn_pid,
		    desc->ptn_cmd);
		if (desc->pl_last)
			level--;
		for (pl = desc->pl_ptn->ptn_children.pl_next;
		     pl != NULL;
		     pl = pl->pl_next) {
			if ((next = malloc(sizeof(*next))) == NULL)
				err(EX_OSERR, NULL);
			next = desc->pl_next;
			desc->pl_next = next;
		}
		if (desc->pl_ptn->ptn_children.pl_next != NULL)
			level++;
		next = desc->pl_next;
		free(desc);
	}
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

	(void)fprintf(stderr, "usage: %s [-a] [pid ...]\n", __progname);
	exit(EX_USAGE);
}
