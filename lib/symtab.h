/* $Id$ */

#include <sys/types.h>
#include <sys/stat.h>

#include <elf_abi.h>

struct symtab {
	FILE				*st_fp;
	struct stat			 st_st;
	int				 st_type;
	union symtab_hdr {
		Elf_Ehdr		 hdr_elf;
	} st_hdr;
	union symtab_data {
		struct symtab_data_elf {
			Elf_Shdr	*elf_shdrs;
			char		*elf_shnams;
			char		*elf_symnams;
			int		 elf_nsyms;
			off_t		 elf_stpos;
		} dat_elf;
	} st_data;
#define st_ehdr		st_hdr.hdr_elf
#define st_eshdrs	st_data.dat_elf.elf_shdrs
#define st_eshnams	st_data.dat_elf.elf_shnams
#define st_esymnams	st_data.dat_elf.elf_symnams
#define st_ensyms	st_data.dat_elf.elf_nsyms
#define st_estpos	st_data.dat_elf.elf_stpos
};

struct symtab	*symtab_open(char *);
const char	*symtab_getsym(struct symtab *, unsigned long);
void		 symtab_close(struct symtab *);
