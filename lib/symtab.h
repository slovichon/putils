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
			Elf_Shdr	 *elf_shdrs;
		} dat_elf;
	} st_data;
#define st_ehdr		st_hdr.hdr_elf
#define st_shdrs	st_data.dat_elf.elf_shdrs
};

struct symtab	*symtab_open(char *);
const char	*symtab_getsym(struct symtab *, unsigned long);
void		 symtab_close(struct symtab *);
