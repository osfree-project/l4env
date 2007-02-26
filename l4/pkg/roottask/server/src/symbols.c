/* Extract symbols information from ELF binary and tell Fiasco where
 * to find it */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <l4/demangle/demangle.h>
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/util/elf.h>
#include <l4/util/l4_macros.h>
#include "cfg.h"
#include "exec.h"
#include "init.h"
#include "memmap.h"
#include "symbols.h"
#include "version.h"

extern void reset_malloc(void);

static void
extract_symbols(l4_addr_t elf_image, unsigned sh_num, unsigned sh_entsize,
		unsigned sh_offs, unsigned task_no,
		l4_addr_t *from_sym, l4_addr_t *to_sym)
{
#if !defined L4BID_RELEASE_MODE
#if defined ARCH_x86 | ARCH_amd64
  int i;
  MY_SHDR *sh_sym, *sh_str;
  unsigned sz_str, sz_sym;
  if (l4_version != VERSION_FIASCO)
    return;

  for (i=0; i<sh_num; i++)
    {
      MY_SYM *sym_symtab;
      const char *sym_strtab;

      sh_sym = (MY_SHDR*)(elf_image + sh_offs + i*sh_entsize);
      if (sh_sym->sh_type == SHT_SYMTAB)
	{
	  unsigned num_symtab;
	  MY_SYM *sym;
	  unsigned size, len;
	  unsigned pages;
	  char *str;
	  l4_addr_t addr, syms;

	  sh_str     = (MY_SHDR*)(elf_image + sh_offs
					       + sh_sym->sh_link*sh_entsize);
	  sz_str     = sh_str->sh_size;
	  if (elf_image)
	    sym_strtab = (char*)(elf_image + sh_str->sh_offset);
	  else
	    sym_strtab = (char*)sh_str->sh_addr;
	  sz_sym     = sh_sym->sh_size;
	  if (elf_image)
	    sym_symtab = (MY_SYM*)(elf_image + sh_sym->sh_offset);
	  else
	    sym_symtab = (MY_SYM*)sh_sym->sh_addr;
	  num_symtab = sz_sym/sizeof(MY_SYM);
	  size       = 0;

	  reset_malloc();
	  // first determine size of symbol table
	  for (i=0, sym=sym_symtab; i<num_symtab; i++, sym++)
	    {
	      const char *s_name = sym_strtab + sym->st_name;
	      const char *d      = cplus_demangle(s_name,
						  DMGL_ANSI | DMGL_PARAMS);
	      //if (d)
		//s_name = d;

	      // ignore something special
	      if (  (sym->st_shndx >= SHN_LORESERVE)
		  ||(sym->st_value == 0)
		  ||(*s_name == '\0')
		  ||(!memcmp(s_name, "Letext", 6))
		  ||(!memcmp(s_name, "_stext", 6)))
		continue;

	      len = strlen(s_name);
	      if (len>100)
		len = 100;
	      size += OFFSET+len+1;

	      if (d)
		free((void*)d);
	    }

	  size += 1; // terminating 0
	  pages = l4_round_page(size)/L4_PAGESIZE;
	  if (0 == (addr = syms = find_free_chunk(pages, 1)))
	    {
	      printf("      No space for symbols left\n");
	      goto done;
	    }
	  str  = (char*)addr;

	  reset_malloc();
	  // now copy symbols into area
	  for (i=0, sym=sym_symtab; i<num_symtab; i++, sym++)
	    {
	      const char *s_name = sym_strtab + sym->st_name;
	      const char *d      = cplus_demangle(s_name,
						  DMGL_ANSI | DMGL_PARAMS);
	      //if (d)
		//s_name = d;

	      // ignore something special
	      if (  (sym->st_shndx >= SHN_LORESERVE)
		  ||(sym->st_value == 0)
		  ||(*s_name == '\0')
		  ||(!memcmp(s_name, "Letext", 6))
		  ||(!memcmp(s_name, "_stext", 6)))
		continue;

	      str += sprintf(str, l4_addr_fmt"   %.100s\n",
		  	     (l4_addr_t)sym->st_value, s_name);

	      if (d)
		free((void*)d);
	    }

	  // terminate the whole list
	  *str = '\0';

	  // mark pages as reserved
	  for (i=0; i<pages; i++, addr+=L4_PAGESIZE)
	    assert(memmap_alloc_page(addr, O_DEBUG));

	  // now publish symbols to fiasco
	  asm ("int $3 ; cmpb $30,%%al" : : "a"(syms), "d"(size),
					    "b" (task_no), "c" (1));

	  *from_sym = syms;
	  *to_sym   = syms+L4_PAGESIZE*pages;

	  goto done;
	}
    }
done:
  ;
#endif
#endif
}

void
extract_symbols_from_image(l4_addr_t elf_image, unsigned task_no,
			   l4_addr_t *from_sym, l4_addr_t *to_sym)
{
  MY_EHDR *ehdr = (MY_EHDR*)elf_image;

  extract_symbols(elf_image, ehdr->e_shnum, ehdr->e_shentsize,
		  ehdr->e_shoff, task_no, from_sym, to_sym);
}

void
extract_symbols_from_mbinfo(l4util_mb_info_t *mbi, unsigned task_no,
			    l4_addr_t *from_sym, l4_addr_t *to_sym)
{
  MY_EHDR *ehdr = (MY_EHDR*)(l4_addr_t)mbi->syms.e.addr;

  extract_symbols(mbi->syms.e.addr, ehdr->e_shnum, ehdr->e_shentsize,
		  ehdr->e_shoff, task_no, from_sym, to_sym);
}
