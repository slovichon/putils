/* $Id$ */

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "est.h"

static __dead void usage(void);

int
main(int argc, char *argv[])
{
	struct elfsymtab *est;
	unsigned long addr;

	if (argc != 3)
		usage();

	addr = strtoul(argv[2], NULL, 10);

	if ((est = est_open(argv[1])) != NULL) {
		(void)printf("%lu: %s\n", addr, est_symgetname(est, addr));
		est_close(est);
	} else
		warnx("%s: not ELF", argv[1]);
	exit(0);
}

static __dead void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s elf-file addr\n", __progname);
	exit(EX_USAGE);
}
