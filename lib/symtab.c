/* $Id$ */

#include <stdio.h>
#include <elf_abi.h>

#include "est.h"

int
est_open(struct elfsymtab *est, char *fil)
{

	if ((est->est_fp = fopen(fil, "r")) == NULL)
		return (0);
	if (fread(&est->est_ehdr, 1, sizeof(est->est_ehdr), est->est_fp) != 1)
		goto notelf;
	if (est->est_ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
	    est->est_ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
	    est->est_ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
	    est->est_ehdr.e_ident[EI_MAG3] != ELFMAG3)
		goto notelf;
	if (est->est_ehdr.e_ident[EI_CLASS] != ELFCLASS32 &&
	    est->est_ehdr.e_ident[EI_CLASS] != ELFCLASS64)
		goto notelf;
	if (est->est_ehdr.e_ident[EI_DATA] != ELFDATA2LSB &&
	    est->est_ehdr.e_ident[EI_DATA] != ELFDATA2MSB)
		goto notelf;
	if (est->est_ehdr.e_ident[EI_VERSION] != EV_CURRENT)
		goto notelf;
	if (est->est_ehdr.e_type != ET_EXEC)
		goto notelf;
	if (est->est_ehdr.e_shoff == 0L)
		goto notelf;
	if (fseek(est->est_fp, est->est_ehdr.e_shoff, SEEK_SET) != 0)
		goto notelf;
	if (fread(&est->est_shdr, 1, sizeof(est->est_shdr), est->est_fp) != 1)
		goto notelf;
	return (1);

notelf:
	(void)fclose(est->est_fp);
	return (0);
}

void
est_close(struct elfsymtab *est)
{

	if (est->est_fp != NULL)
		(void)fclose(est->est_fp);
}

char *
est_symgetname(struct elfsymtab *est, unsigned long addr)
{
	char *s;

	s = NULL;

	return (s);
}
