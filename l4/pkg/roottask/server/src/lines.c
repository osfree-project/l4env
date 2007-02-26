/* Extract debug lines info from an ELF binary stab section and tell Fiasco
 * where to find it */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <l4/sys/types.h>
#include <l4/sys/kdebug.h>
#include <l4/util/elf.h>

#include "memmap.h"
#include "lines.h"
#include "rmgr.h"
#include "version.h"

#define MAX_PAGES		512
#define LIN_PER_PAGE		(L4_PAGESIZE/sizeof(stab_entry_t))

typedef struct
{
  unsigned long n_strx;         /* index into string table of name */
  unsigned char n_type;         /* type of symbol */
  unsigned char n_other;        /* misc info (usually empty) */
  unsigned short n_desc;        /* description field */
  unsigned long n_value;        /* value of symbol */
} __attribute__ ((packed)) stab_entry_t;

typedef struct
{
  unsigned long addr;
  unsigned short line;
} __attribute__ ((packed)) stab_line_t;

#if !defined L4BID_RELEASE_MODE
#ifdef ARCH_x86
static char *str_field;
static stab_line_t *lin_field;
static l4_addr_t tmp_mem;
static l4_addr_t str_end, str_max;
static l4_addr_t lin_end, lin_max;
static l4_offs_t func_offset;
static int have_func;

static Elf32_Shdr*
elf_sh_lookup(l4_addr_t sh_addr, l4_size_t sh_entsize, int num, int n)
{
  return (n < num)
    ? (Elf32_Shdr*)(sh_addr + n*sh_entsize)
    : 0;
}

static int
search_str(const char *str, unsigned len, unsigned *idx)
{
  const char *c_next, *c, *d;

  if (!str_field || !*str_field)
    return 0;

  for (c_next=str_field; ; )
    {
      // goto end of string
      for (; *c_next; c_next++)
	;

      // compare strings
      for (c=c_next, d=str+len; d>=str && (*c == *d); c--, d--)
	;

      if (d<str)
	{
	  *idx = c+1-str_field;
	  // found (could be a substring)
	  return 1;
	}

      // test for end-of-field
      if (!*++c_next)
	// end of field -- not found
	return 0;
    }
}

static int
add_str(const char *str, unsigned len, unsigned *idx)
{
  // if still included, don't include anymore
  if (search_str(str, len, idx))
    return 1;

  // check if str_field is big enough
  if (str_end+len+1 >= str_max)
    {
      printf("      Not enough memory for debug lines (str space)\n");
      return 0;
    }

  *idx = str_end;

  // add string to end of str_field
  strcpy(str_field+str_end, str);
  str_end += len+1;

  // terminate string field
  str_field[str_end] = '\0';

  return 1;
}

// see documentation in "info stabs"
static int
add_entry(const stab_entry_t *se, const char *se_str)
{
  const char *se_name = se_str + se->n_strx;
  int se_name_len = strlen(se_name);
  unsigned se_name_idx = 0;

  if (se_name_len
      && ((se->n_type == 0x64) || (se->n_type == 0x84)))
    {
      if (!add_str(se_name, se_name_len, &se_name_idx))
	return 0;
    }

  if (lin_end >= lin_max)
    {
      printf("      Not enough temporary memory for lines (lin space)\n");
      return 0;
    }

  switch (se->n_type)
    {
    case 0x24: // N_FUN: function name
      func_offset = se->n_value;		// start address of function
      have_func   = 1;
      break;
    case 0x44: // N_SLINE: line number in text segment
      if (have_func && (se->n_desc < 0xfffd)) // sanity check
	{
	  // Search last SLINE entry and compare addresses. If they are
	  // same, overwrite the last entry because we display only one
	  // line number per address
	  unsigned addr = se->n_value + func_offset;

	  unsigned l = lin_end;
	  while ((l > 0) && (lin_field[l-1].line>=0xfffd))
	    l--;

	  if ((l > 0) && (lin_field[l-1].addr == addr))
	    {
	      // found, delete array entry
	      while (l < lin_end)
		{
		  lin_field[l-1] = lin_field[l];
		  l++;
		}
	      lin_end--;
	    }
	  // append
	  lin_field[lin_end].addr = addr;
     	  lin_field[lin_end].line = se->n_desc;
	  lin_end++;
	}
      break;
    case 0x64: // N_SO: main file name / directory
      if (se_name_len)
	{
	  if (lin_end>0 && lin_field[lin_end-1].line == 0xffff)
	    {
	      // mark last entry as directory, this is a file name
	      lin_field[lin_end-1].line = 0xfffe;
	    }
	}
      else
	have_func = 0;
      // fall through
    case 0x84: // N_SOL: name of include file
      if (se_name_len)
	{
	  int type = 0xffff;
	  char c = se_name[se_name_len-1];

	  if (c=='h' || c=='H')
	    type = 0xfffd; // mark as header file

	  lin_field[lin_end].addr = se_name_idx;
	  lin_field[lin_end].line = type;
	  lin_end++;
	}
      break;
    }

  return 1;
}

static int
add_section(const stab_entry_t *se, const char *str, unsigned n)
{
  unsigned int i;

  func_offset = 0;
  have_func   = 0;

  for (i=0; i<n; i++)
    if (!add_entry(se++, str))
      return 0;

  return 1;
}
#endif
#endif

static void
extract_lines(unsigned elf_image, unsigned sh_num, unsigned sh_entsize,
	      unsigned sh_offs, unsigned sh_strndx, unsigned task_no,
	      l4_addr_t *from_lin, l4_addr_t *to_lin)
{
#if !defined L4BID_RELEASE_MODE
#ifdef ARCH_x86
  Elf32_Shdr *sh_sym, *sh_str;
  const char *strtab;
  int i;

  if (l4_version != VERSION_FIASCO)
    return;

  // search contiguous free pages for temporary memory
  if (0 == (tmp_mem = find_free_chunk(2*MAX_PAGES, 0)))
    {
      printf("      No space for temporary memory for lines left\n");
      return;
    }
  lin_field = (stab_line_t*)tmp_mem;
  lin_max   = MAX_PAGES*L4_PAGESIZE/sizeof(stab_line_t);
  str_field = (char*)(tmp_mem + MAX_PAGES*L4_PAGESIZE);
  str_max   = MAX_PAGES*L4_PAGESIZE;
  str_end   = lin_end = 0;

  str_field[0] = '\0';

  if (sh_strndx == SHN_UNDEF)
    return;

  if (0 == (sh_str = elf_sh_lookup(elf_image+sh_offs, sh_entsize, sh_num,
				   sh_strndx)))
    {
      printf("      Error in ELF stab section\n");
      return;
    }

  if (elf_image)
    strtab = (const char*)(elf_image + sh_str->sh_offset);
  else
    strtab = (const char*)sh_str->sh_addr;

  for (i=0; i<sh_num; i++)
    {
      sh_sym = elf_sh_lookup(elf_image+sh_offs, sh_entsize, sh_num, i);
      if (   (   sh_sym->sh_type == SHT_PROGBITS
	      || sh_sym->sh_type == SHT_STRTAB)
	  && (!strcmp(sh_sym->sh_name + strtab, ".stab")))
	{
	  const stab_entry_t *se;
	  const char *str;

	  if (0 == (sh_str = elf_sh_lookup(elf_image+sh_offs, sh_entsize,
					   sh_num, sh_sym->sh_link)))
	    {
	      printf("      Error in ELF stab section\n");
	      return;
	    }

	  if (elf_image)
	    {
	      se  = (const stab_entry_t*)(elf_image+sh_sym->sh_offset);
	      str = (const char*)        (elf_image+sh_str->sh_offset);
	    }
	  else
	    {
	      se  = (const stab_entry_t*)sh_sym->sh_addr;
	      str = (const char*)        sh_str->sh_addr;
	    }
	  if (!add_section(se, str, sh_sym->sh_size/sh_sym->sh_entsize))
	    return;
	}
    }

  if (lin_end > 0)
    {
      l4_addr_t addr, lines;
      l4_umword_t pages;
      l4_size_t size;

      // add terminating entry
      if (lin_end >= lin_max)
	{
	  printf("      Not enough temporary memory for lines "
	         "(terminating line entry)\n");
      	  return;
	}
      lin_field[lin_end].addr = 0;
      lin_field[lin_end].line = 0;
      lin_end++;

      // string offset relativ to begin of lines
      for (i=0; i<lin_end; i++)
	if (lin_field[i].line >= 0xfffd)
	  lin_field[i].addr += lin_end*sizeof(stab_line_t);

      size = str_end + lin_end*sizeof(stab_line_t);
      pages = l4_round_page(size) / L4_PAGESIZE;

      // now reserve space at end of physcial memory
      if (0 == (lines = addr = find_free_chunk(pages, 1)))
	{
	  printf("      No space for lines left\n");
	  return;
	}

      memcpy((void*)lines, lin_field, lin_end*sizeof(stab_line_t));
      memcpy((void*)(lines+lin_end*sizeof(stab_line_t)), str_field, str_end);

      // mark pages as reserved
      for (i=0; i<pages; i++, addr+=L4_PAGESIZE)
	assert(memmap_alloc_page(addr, O_DEBUG));

      asm ("int $3 ; cmpb $30,%%al" : : "a"(lines), "d"(size),
					"b" (task_no), "c" (2));

      *from_lin = lines;
      *to_lin   = lines + pages*L4_PAGESIZE;

      return;
    }
#endif
#endif
}

void
extract_lines_from_image(l4_addr_t elf_image, unsigned task_no,
			 l4_addr_t *from_lin, l4_addr_t *to_lin)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)elf_image;

  extract_lines(elf_image, ehdr->e_shnum, ehdr->e_shentsize,
                ehdr->e_shoff, ehdr->e_shstrndx, task_no, from_lin, to_lin);
}

void
extract_lines_from_mbinfo(l4util_mb_info_t *mbi, unsigned task_no,
			  l4_addr_t *from_lin, l4_addr_t *to_lin)
{
  Elf32_Ehdr *ehdr = (Elf32_Ehdr*)(l4_addr_t)mbi->syms.e.addr;

  extract_lines(mbi->syms.e.addr, ehdr->e_shnum, ehdr->e_shentsize,
                ehdr->e_shoff, ehdr->e_shstrndx, task_no, from_lin, to_lin);
}

