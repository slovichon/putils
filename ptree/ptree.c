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
	char 		*ptn_cmd;
	struct plink	 ptn_children;
};

static		char  *join(char **);
static		void   buildtree(struct ptnode **);
static		void   dotree(struct ptnode *, char *);
static		void   freetree(struct ptnode *);
struct		plink *findpl(struct plink *, pid_t);
static __dead	void   usage(void);

static int		 showinit = 0;

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
			dotree(root, *argv);
	else
		dotree(root, NULL);
	freetree(root);
	exit(EXIT_SUCCESS);
}

static void
buildtree(struct ptnode **t)
{
	char **argv, buf[_POSIX2_LINE_MAX];
	struct plink unacc, *pl, *plchild;
	struct ptnode *ptn, *ptnchild;
	struct kinfo_proc *kip;
	struct proc *child;
	int i, pcnt;
	kvm_t *kd;

	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL, (char *)NULL,
	     KVM_NO_FILES, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	kip = kvm_getprocs(kd, KERN_PROC_KTHREAD, 0, &pcnt);
	if (kip == NULL || pcnt == 0)
		errx(EX_OSERR, "kvm_getprocs: %s", kvm_geterr(kd));
	unacc.pl_next = NULL;
	for (i = 0; i < pcnt; i++, kip++) {
		if ((argv = kvm_getargv(kd, kip, 0)) == NULL)
			err(EX_OSERR, "kvm_getargv: %s", kvm_geterr(kd));
		if ((pl = findpl(&unacc, kip->kp_proc.p_pid)) == NULL) {
			if ((ptn = malloc(sizeof(*ptn))) == NULL)
				err(EX_OSERR, "malloc");
			/* Add to list of unaccounted nodes. */
			if ((pl = malloc(sizeof(*pl))) == NULL)
				err(EX_OSERR, "malloc");
			pl->pl_next = unacc.pl_next;
			pl->pl_ptn = ptn;
			unacc.pl_next = pl;
		} else {
			/*
			 * A node for this process has already
			 * been allocated, so use it.
			 */
			ptn = pl->pl_next->pl_ptn;
		}
		ptn->ptn_children.pl_next = NULL;
		ptn->ptn_cmd = join(argv);
		ptn->ptn_pid = kip->kp_proc.p_pid;
		for (child = kip->kp_proc.p_children.lh_first;
		     child != NULL; child = child->p_sibling.le_next) {
			if ((pl = findpl(&unacc, child->p_pid)) == NULL) {
				/* Child not found; allocate new. */
				if ((plchild = malloc(sizeof(*pl))) == NULL)
					err(EX_OSERR, "malloc");
				if ((ptnchild = malloc(sizeof(*ptnchild))) ==
				    NULL)
					err(EX_OSERR, "malloc");
				ptnchild->ptn_cmd = NULL;
				ptnchild->ptn_pid = child->p_pid;
				ptnchild->ptn_children.pl_next = NULL;
				plchild->pl_ptn = ptnchild;
			} else {
				/*
			 	* Child found; it is no longer
			 	* unaccounted for.
			 	*/
				plchild = pl->pl_next;
				pl->pl_next = plchild->pl_next;
			}
			plchild->pl_next = ptn->ptn_children.pl_next;
			ptn->ptn_children.pl_next = plchild;
		}
	}
	(void)kvm_close(kd);
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
	for (t = s; *t != NULL; t++)
		strlcat(p, *t, siz);
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
dotree(struct ptnode *ptn, char *s)
{
	pid_t pid;

	if (s != NULL && !parsepid(s, &pid)) {
		xwarn("cannot examine %s", s);
		return;
	}
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s [-a] [pid|user ...]\n", __progname);
	exit(EX_USAGE);
}
