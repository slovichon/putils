/* $Id$ */

#include <sys/types.h>
#include <sys/stat.h>

#include <elf_abi.h>
#include <stdio.h>
#include <stdlib.h>

#include "est.h"

struct elfsymtab *
est_open(char *fil)
{
	struct elfsymtab *est;

	if ((est = malloc(sizeof(*est))) == NULL)
		return (NULL);
	if ((est->est_fp = fopen(fil, "r")) == NULL)
		goto notelf;
	if (fstat(fileno(est->est_fp), &est->est_st) == -1)
		goto notelf;
	if (fread(&est->est_ehdr, sizeof(est->est_ehdr), 1, est->est_fp) != 1)
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
	if ((est->est_shdrs = malloc(est->est_ehdr.e_shentsize *
	     est->est_ehdr.e_shnum)) == NULL)
		goto notelf;
	if (fseek(est->est_fp, est->est_ehdr.e_shoff, SEEK_SET) == -1) {
	    	free(est->est_shdrs);
		goto notelf;
	}
	if (fread(est->est_shdrs, est->est_ehdr.e_shentsize,
	    est->est_ehdr.e_shnum, est->est_fp) != est->est_ehdr.e_shnum) {
	    	free(est->est_shdrs);
		goto notelf;
	}
	return (est);

notelf:
	if (est->est_fp != NULL)
		(void)fclose(est->est_fp);
	free(est);
	return (NULL);
}

void
est_close(struct elfsymtab *est)
{
	(void)fclose(est->est_fp);
	free(est->est_shdrs);
	free(est);
}

char *
est_symgetname(struct elfsymtab *est, unsigned long addr)
{
	Elf_Shdr *strhdr;
	char *s, *shnams, *symnams;
	int i, j, nsyms;
	Elf_Sym esym;
	size_t siz;

	shnams = NULL;
	symnams = NULL;
	s = NULL;

	/* Get the names of the sections. */
	strhdr = &est->est_shdrs[est->est_ehdr.e_shstrndx];
#if 0
	/* XXX: check for spoofed e_shstrndx. */
	if (est->est_ehdr.e_shstrndx > est->est_ehdr.e_shnum)
		/* XXX: kill est obj, because it is invalid. */
		goto end;
#endif
	siz = strhdr->sh_size;
	if ((shnams = malloc(siz)) == NULL)
		goto end;
	if (fseek(est->est_fp, strhdr->sh_offset, SEEK_SET) == -1)
		goto end;
	if (fread(shnams, 1, siz, est->est_fp) != siz)
		goto end;

	/* Find the string table section. */
	for (i = 0; i < est->est_ehdr.e_shnum; i++)
		if (strcmp(shnams + est->est_shdrs[i].sh_name,
		    ELF_STRTAB) == 0)
			break;
	if (i == est->est_ehdr.e_shnum)
		goto end;
	if ((symnams = malloc(est->est_shdrs[i].sh_size)) == NULL)
		goto end;
	if (fseek(est->est_fp, est->est_shdrs[i].sh_offset, SEEK_SET) == -1)
		goto end;
	if (fread(symnams, 1, est->est_shdrs[i].sh_size, est->est_fp) !=
	    est->est_shdrs[i].sh_size)
		goto end;

	/* Find the symbol table section. */
	for (i = 0; i < est->est_ehdr.e_shnum; i++)
		if (strcmp(shnams + est->est_shdrs[i].sh_name,
		    ELF_SYMTAB) == 0)
			break;
	if (i == est->est_ehdr.e_shnum)
		goto end;

	/* Find desired symbol in table. */
	if (fseek(est->est_fp, est->est_shdrs[i].sh_offset, SEEK_SET) == -1)
		goto end;
	nsyms = est->est_shdrs[i].sh_size / sizeof(esym);
	for (j = 0; j < nsyms; j++) {
		if (fread(&esym, 1, sizeof(esym), est->est_fp) !=
		    sizeof(esym))
			goto end;
		if (ELF_ST_TYPE(esym.st_info) == STT_FUNC) {
#if 0
			/* Check for spoofing. */
			if ((long long)esym.st_name > est->est_st.st_size)
				continue;
#endif
			/* XXX: check for st_name < sh_size */
			printf("%s: %d\n", &symnams[esym.st_name], esym.st_value);
		}
	}

end:
	free(symnams);
	free(shnams);
	return (s);
}
