/* $Id$ */

#include <sys/types.h>
#include <sys/stat.h>

#include <elf_abi.h>

struct elfsymtab {
	FILE		*est_fp;
	struct stat	 est_st;
	Elf_Ehdr	 est_ehdr;
	Elf_Shdr	*est_shdrs;
};

struct elfsymtab	*est_open(char *);
char			*est_symgetname(struct elfsymtab *, unsigned long);
void			 est_close(struct elfsymtab *);
