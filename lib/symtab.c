/* $Id$ */

#include <sys/types.h>
#include <sys/stat.h>

#include <elf_abi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtab.h"

static int		 symtab_elf_open(struct symtab *);
static int		 symtab_elf_probe(struct symtab *);
static const char	*symtab_elf_getsym(struct symtab *, unsigned long);

static struct binsw {
	int (*open)(struct symtab *);
	int (*probe)(struct symtab *);
	const char *(*getsym)(struct symtab *, unsigned long);
} binsw[] = {
	{ symtab_elf_probe, symtab_elf_open, symtab_elf_getsym }
};
#define NBINSW (sizeof(binsw) / sizeof(binsw[0]))

struct symtab *
symtab_open(char *fil)
{
	struct symtab *st;
	int i;

	if ((st = malloc(sizeof(*st))) == NULL)
		return (NULL);
	if ((st->st_fp = fopen(fil, "r")) == NULL)
		goto notbin;
	if (fstat(fileno(st->st_fp), &st->st_st) == -1)
		goto notbin;
	/* XXX: sloppy */
	if (fread(&st->st_hdr, 1, sizeof(st->st_hdr), st->st_fp) !=
	    sizeof(st->st_hdr))
		goto notbin;
	for (i = 0; i < NBINSW; i++)
		if (binsw[i].probe(st))
			break;
	if (i == NBINSW)
		goto notbin;
	if (!binsw[i].open(st))
		goto notbin;
	st->st_type = i;
	return (st);

notbin:
	if (st->st_fp != NULL)
		(void)fclose(st->st_fp);
	free(st);
	return (NULL);
}

static int
symtab_elf_probe(struct symtab *st)
{
	if (st->st_ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
	    st->st_ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
	    st->st_ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
	    st->st_ehdr.e_ident[EI_MAG3] != ELFMAG3)
		return (0);
	if (st->st_ehdr.e_ident[EI_CLASS] != ELFCLASS32 &&
	    st->st_ehdr.e_ident[EI_CLASS] != ELFCLASS64)
		return (0);
	if (st->st_ehdr.e_ident[EI_DATA] != ELFDATA2LSB &&
	    st->st_ehdr.e_ident[EI_DATA] != ELFDATA2MSB)
		return (0);
	if (st->st_ehdr.e_ident[EI_VERSION] != EV_CURRENT)
		return (0);
	if (st->st_ehdr.e_type != ET_EXEC)
		return (0);
	if (st->st_ehdr.e_shoff == 0L)
		return (0);
	return (1);
}

static int
symtab_elf_open(struct symtab *st)
{
	if ((st->st_shdrs = malloc(st->st_ehdr.e_shentsize *
	     st->st_ehdr.e_shnum)) == NULL)
		return (0);
	if (fseek(st->st_fp, st->st_ehdr.e_shoff, SEEK_SET) == -1) {
		free(st->st_shdrs);
		return (0);
	}
	if (fread(st->st_shdrs, st->st_ehdr.e_shentsize,
	    st->st_ehdr.e_shnum, st->st_fp) != st->st_ehdr.e_shnum) {
		free(st->st_shdrs);
		return (0);
	}
	return (1);
}

void
symtab_close(struct symtab *st)
{
	(void)fclose(st->st_fp);
	free(st->st_shdrs);
	free(st);
}

const char *
symtab_getsym(struct symtab *st, unsigned long addr)
{
	return (binsw[st->st_type].getsym(st, addr));
}

static const char *
symtab_elf_getsym(struct symtab *st, unsigned long addr)
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
	strhdr = &st->st_shdrs[st->st_ehdr.e_shstrndx];
#if 0
	/* XXX: check for spoofed e_shstrndx. */
	if (st->st_ehdr.e_shstrndx > st->st_ehdr.e_shnum)
		/* XXX: kill st obj, because it is invalid. */
		goto end;
#endif
	siz = strhdr->sh_size;
	if ((shnams = malloc(siz)) == NULL)
		goto end;
	if (fseek(st->st_fp, strhdr->sh_offset, SEEK_SET) == -1)
		goto end;
	if (fread(shnams, 1, siz, st->st_fp) != siz)
		goto end;

	/* Find the string table section. */
	for (i = 0; i < st->st_ehdr.e_shnum; i++)
		if (strcmp(shnams + st->st_shdrs[i].sh_name,
		    ELF_STRTAB) == 0)
			break;
	if (i == st->st_ehdr.e_shnum)
		goto end;
	if ((symnams = malloc(st->st_shdrs[i].sh_size)) == NULL)
		goto end;
	if (fseek(st->st_fp, st->st_shdrs[i].sh_offset, SEEK_SET) == -1)
		goto end;
	if (fread(symnams, 1, st->st_shdrs[i].sh_size, st->st_fp) !=
	    st->st_shdrs[i].sh_size)
		goto end;

	/* Find the symbol table section. */
	for (i = 0; i < st->st_ehdr.e_shnum; i++)
		if (strcmp(shnams + st->st_shdrs[i].sh_name,
		    ELF_SYMTAB) == 0)
			break;
	if (i == st->st_ehdr.e_shnum)
		goto end;

	/* Find desired symbol in table. */
	if (fseek(st->st_fp, st->st_shdrs[i].sh_offset, SEEK_SET) == -1)
		goto end;
	nsyms = st->st_shdrs[i].sh_size / sizeof(esym);
	for (j = 0; j < nsyms; j++) {
		if (fread(&esym, 1, sizeof(esym), st->st_fp) !=
		    sizeof(esym))
			goto end;
		if (ELF_ST_TYPE(esym.st_info) == STT_FUNC) {
#if 0
			/* Check for spoofing. */
			if ((long long)esym.st_name > st->st_st.st_size)
				continue;
#endif
			/* XXX: check for st_name < sh_size */
			if (esym.st_value == addr)
				return (&symnams[esym.st_name]);
		}
	}

end:
	free(symnams);
	free(shnams);
	return (s);
}
