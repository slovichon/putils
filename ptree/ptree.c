/* $Id$ */

#include <sys/proc.h>
#include <sys/queue.h>

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "putils.h"
#include "util.h"

struct ptnode {
	pid_t		 ptn_pid;
	char 		*ptn_cmd;
	struct plink	*ptn_children;
};

struct plink {
	struct ptnode	*pl_ptn;
	struct plink	*pl_next;
};

static		void  buildtree(struct ptnode **);
static		char *join(char **);
static __dead	void  usage(void);

static int		 showinit = 0;

int
main(int argc, char *argv[])
{
	char buf[_POSIX2_LINE_MAX];
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
	freetree(&root);
	exit(EXIT_SUCCESS);
}

static void
buildtree(struct ptnode **t)
{
	struct plink unacc, *pl, *plchild, *pl;
	struct ptnode *ptn, *ptnchild;
	struct kinfo_proc *kip;
	struct proc *child;
	char **argv;
	kvm_t *kd;
	int i;

	if ((kd = kvm_openfiles((char *)NULL, (char *)NULL, (char *)NULL,
	    0, buf)) == NULL)
		errx(EX_OSERR, "kvm_openfiles: %s", buf);
	kip = kvm_getproc(kd, KERN_PROC_KTHREAD, 0, &pcnt);
	if (kip == NULL || pcnt == 0)
		errx(EX_OSERR, "kvm_getproc2: %s", kvm_geterr(kd));
	unacc.pl_next = NULL;
	for (i = 0; i < pcnt; i++, kip++) {
		if ((argv = kvm_getargv(kd, kip, 0)) == NULL)
			err(EX_OSERR, "kvm_getargv2: ", kvm_geterr(kde));
		if ((pl = findpl(&unacc, kip->kp_proc.p_pid)) == NULL)
			if ((ptn = malloc(sizeof(*ptn))) == NULL)
				err(EX_OSERR, "malloc");
			/* Add to list of unaccounted nodes. */
			if (pl = malloc(sizeof(*pl)) == NULL)
				err(EX_OSERR, "malloc");
			pl->pl_next = unacc.next;
			pl->pl_ptn = ptn;
			unacc.next = pl;
		} else {
			/*
			 * A node for this process has already
			 * been allocated, so use it.
			 */
			ptn = pl->pl->next->pl_ptn;
		}
		plchildren = NULL;
		LIST_FOREACH(child, &kip->kp_proc.p_children, proc) {
			if ((pl = findpl(&unacc, child->p_pid)) == NULL) {
				/* Child not found; allocate new. */
				if ((plchild = malloc(sizeof(*pl))) == NULL)
					err(EX_OSERR, "malloc");
				if ((ptnchild = malloc(sizeof(*ptnchild))) ==
				    NULL)
					err(EX_OSERR, "malloc");
				ptnchild->ptn_cmd = NULL;
				ptnchild->ptn_pid = child->p_pid;
				ptnchild->ptn_children = NULL;
				plchild->pl_ptn = ptnchild;
			} else {
				/*
			 	* Child found; it is no longer
			 	* unaccounted for.
			 	*/
				plchild = pl->pl_next;
				pl->pl_next = plchild->pl_next;
			}
			plchild->pl_next = plchildren;
			plchildren = plchild;
		}
		ptn->ptn_children = plchildren;
		ptn->ptn_cmd = join(argv);
		ptn->ptn_pid = kip->p_pid;
	}
	(void)kvm_close(kd);
}

struct plink *
findpl(struct plink *base, pid_t pid)
{
	struct plink *pl, *plchild;
	struct ptnode *ptn;

	pl = NULL;
	for (; base != NULL; base = base->pl_next) {
		ptn = pl->pl_ptn;
		findpl();
	}
	return pl;
}

char *
join(char **s)
{
	size_t siz;
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
	return p;
}

static void
freetree(struct ptnode *ptn)
{
	struct plink *pl, *next;

	free(ptn->ptn_cmd);
	for (pl = ptn->ptn_child; *pl != NULL; pl = next) {
		next = pl->pl_next;
		freetree(pl->pl_ptn);
		free(pl);
	}
	free(p);
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
