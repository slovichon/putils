/* $Id$ */

#include <elf_abi.h>

struct elfsymtab {
	FILE	*est_fp;
	Elf_Ehdr est_ehdr;
	Elf_Shdr est_shdr;
};

int	 est_open(struct elfsymtab *, char *);
char	*est_symgetname(struct elfsymtab *, unsigned long);
void	 est_close(struct elfsymtab *);
