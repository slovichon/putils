/* $Id$ */

#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "symtab.h"

static void usage(void) __attribute__((__noreturn__));

int
main(int argc, char *argv[])
{
	struct symtab *st;
	unsigned long addr;

	if (argc != 3)
		usage();

	if (argv[2][0] == '0' && argv[2][1] == 'x')
		addr = strtoul(&argv[2][2], NULL, 16);
	else
		addr = strtoul(argv[2], NULL, 10);

	if ((st = symtab_open(argv[1])) != NULL) {
		(void)printf("%lu: %s\n", addr, symtab_getsym(st, addr));
		symtab_close(st);
	} else
		warnx("%s: not a binary executable", argv[1]);
	exit(0);
}

static void
usage(void)
{
	extern char *__progname;

	(void)fprintf(stderr, "usage: %s elf-file addr\n", __progname);
	exit(EX_USAGE);
}
