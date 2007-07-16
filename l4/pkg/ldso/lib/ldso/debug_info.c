/* $Id$ */
/**
 * \file	ldso/lib/ldso/debug_info.c
 * \brief	extract debug info fro binaries
 * 
 * \date	01/2006
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2006 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/consts.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/demangle/demangle.h>
#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <link.h>
#include <elf.h>
#include <string.h>
#include <stdio.h>
#include "dl-syscall.h"

#include "binary_name.h"
#include "debug_info.h"
#include "emul_linux.h"
#include "malloc.h"

#define SCRATCH_SIZE	(8<<20)

#if DEBUG_LEVEL>0
#define DBG 1
#else
#define DBG 0
#endif

typedef struct
{
  unsigned int   n_strx;        /* index into string table of name */
  unsigned char  n_type;        /* type of symbol */
  unsigned char  n_other;       /* misc info (usually empty) */
  unsigned short n_desc;        /* description field */
  unsigned int   n_value;       /* value of symbol */
} __attribute__ ((packed)) stab_entry_t;

typedef struct
{
  unsigned long  addr;
  unsigned short line;
} __attribute__ ((packed)) stab_line_t;

typedef struct
{
  char               *symbols_addr;
  l4_size_t           symbols_size;
  char               *lines_addr_l;
  char               *lines_addr_s;
  l4_size_t           lines_size_l;
  l4_size_t           lines_size_s;
  struct elf_resolve *tpnt;
} debug_info_t;


static debug_info_t debug_info[32];
static unsigned     nr_debug_info;
static char        *glob_symbols;
static l4_size_t    size_symbols;
static char        *glob_lines;
static l4_size_t    size_lines;

static char        *str_field;
static stab_line_t *lin_field;
static l4_addr_t    str_end, str_max;
static l4_addr_t    lin_end, lin_max;
static l4_offs_t    func_offset;
static int          have_func;


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
      LOGd(DBG, "Not enough memory for debug lines (str space)");
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
add_entry(const stab_entry_t *se, const char *se_str, l4_addr_t libaddr)
{
  const char *se_name = se_str + se->n_strx;
  int se_name_len = strlen(se_name);
  unsigned se_name_idx = 0;

  if (se_name_len && (se->n_type == 0x64 || se->n_type == 0x84))
    {
      if (!add_str(se_name, se_name_len, &se_name_idx))
	return 0;
    }

  if (lin_end >= lin_max)
    {
      LOGd(DBG, "Not enough temporary memory for lines (lin space)");
      return 0;
    }

  switch (se->n_type)
    {
    case 0x24: // N_FUN: function name
      func_offset = se->n_value + libaddr; // start address of function
      have_func   = 1;
      break;
    case 0x44: // N_SLINE: line number in text segment
      if (have_func && (se->n_desc < 0xfffd)) // sanity check
	{
	  // Search last SLINE entry and compare addresses. If they are
	  // same, overwrite the last entry because we display only one
	  // line number per address
	  l4_addr_t addr = se->n_value + func_offset;

	  unsigned l = lin_end;
	  while (l > 0 && lin_field[l-1].line>=0xfffd)
	    l--;

	  if (l > 0 && lin_field[l-1].addr == addr)
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
add_section(const stab_entry_t *se, const char *str,
	    unsigned n, l4_addr_t libaddr)
{
  unsigned i;

  func_offset = 0;
  have_func   = 0;

  for (i=0; i<n; i++, se++)
    if (!add_entry(se, str, libaddr))
      return 0;

  return 1;
}

static void
lines_add(int infile, const char *header, l4_addr_t libaddr,
	  void *scratch_mem, l4_size_t scratch_size,
	  ElfW(Shdr) *sh_base, int nr_lin, debug_info_t *d)
{
  ElfW(Shdr)   *sh_lin = sh_base + nr_lin;
  ElfW(Shdr)   *sh_str = sh_base + sh_lin->sh_link;
  stab_entry_t *lin_stab = 0;
  char         *lin_str = 0;

  if (d->lines_addr_l)
    {
      LOGd(DBG, "More than one .stab section");
      return;
    }

  lin_field = scratch_mem;
  lin_max   = (scratch_size/2)/sizeof(stab_line_t);
  lin_end   = 0;

  str_field = scratch_mem+scratch_size/2;
  str_max   = scratch_size/2;
  str_end   = 0;

  str_field[0] = '\0';

  if (!(lin_stab = _dl_alloc_pages(sh_lin->sh_size, 0, "tmp lin_stab")))
    goto error;
  _dl_seek(infile, sh_lin->sh_offset);
  _dl_read(infile, lin_stab, sh_lin->sh_size);

  if (!(lin_str = _dl_alloc_pages(sh_str->sh_size, 0, "tmp lin_str")))
    goto error;
  _dl_seek(infile, sh_str->sh_offset);
  _dl_read(infile, lin_str, sh_str->sh_size);

  if (!add_section(lin_stab, lin_str, sh_lin->sh_size/sh_lin->sh_entsize,
		   libaddr))
    goto error;

  if (lin_end > 0)
    {
      d->lines_size_l = lin_end*sizeof(stab_line_t);
      d->lines_size_s = str_end;
      if (!(d->lines_addr_l =
	    _dl_alloc_pages(d->lines_size_l, 0, "[lin_stab]")) ||
	  !(d->lines_addr_s =
	    _dl_alloc_pages(d->lines_size_s, 0, "[lin_str]")))
	{
	  _dl_free_pages(d->lines_addr_l, d->lines_size_l);
	  _dl_free_pages(d->lines_addr_s, d->lines_size_s);
	  goto error;
	}
      memcpy(d->lines_addr_l, scratch_mem,                d->lines_size_l);
      memcpy(d->lines_addr_s, scratch_mem+scratch_size/2, d->lines_size_s);
    }

error:
  _dl_free_pages(lin_str,  sh_str->sh_size);
  _dl_free_pages(lin_stab, sh_lin->sh_size);
}

static void
symbols_add(int infile, l4_addr_t libaddr,
	    void *scratch_mem, l4_size_t scratch_size,
	    ElfW(Shdr) *sh_base, int nr_sym, debug_info_t *d)
{
  ElfW(Shdr) *sh_sym = sh_base + nr_sym;
  ElfW(Shdr) *sh_str = sh_base + sh_sym->sh_link;
  char       *sym_strtab = 0;
  ElfW(Sym)  *sym_symtab = 0;
  unsigned    num_symtab = sh_sym->sh_size / sizeof(ElfW(Sym));
  unsigned    size;
  ElfW(Sym)  *sym;
  int         i;
  char       *str = 0;

  if (d->symbols_addr)
    {
      LOGd(DBG, "More than one symbols sections");
      return;
    }

  if (!(sym_strtab = _dl_alloc_pages(sh_str->sh_size, 0, "tmp sym_strtab")))
    goto error;
  _dl_seek(infile, sh_str->sh_offset);
  _dl_read(infile, sym_strtab, sh_str->sh_size);

  if (!(sym_symtab = _dl_alloc_pages(sh_sym->sh_size, 0, "tmp sym_symtab")))
    goto error;
  _dl_seek(infile, sh_sym->sh_offset);
  _dl_read(infile, sym_symtab, sh_sym->sh_size);

  if (!demangle_malloc_reset())
    goto error;

  for (i=0, size=0, sym=sym_symtab, str=scratch_mem; i<num_symtab; i++, sym++)
    {
      const char *s_name = sym_strtab + sym->st_name;
      char *d            = cplus_demangle(s_name, DMGL_ANSI | DMGL_PARAMS);
      int rc;

      if (d)
	s_name = d;

      if (sym->st_shndx < SHN_LORESERVE &&
	  sym->st_value != 0 &&
	  *s_name != '\0' &&
	  memcmp(s_name, "Letext", 6) &&
	  memcmp(s_name, "_stext", 6))
	{
	  if (size > scratch_size-120)
	    {
	      LOGd(DBG, "scratch_mem too small");
	      break;
	    }
	  rc = sprintf(str, l4_addr_fmt"   %.100s\n",
	               sym->st_value+libaddr, s_name);
	  str  += rc;
	  size += rc;
	}
      if (d)
	demangle_free(d);
    }

  if (!(d->symbols_addr = _dl_alloc_pages(size, 0, "[symbols]")))
    goto error;
  memcpy(d->symbols_addr, scratch_mem, size);
  d->symbols_size = size;

error:
  _dl_free_pages(sym_symtab, sh_sym->sh_size);
  _dl_free_pages(sym_strtab, sh_str->sh_size);
}

void
_dl_debug_info_add(struct elf_resolve *tpnt, int infile,
		   const char *header, l4_addr_t libaddr)
{
  ElfW(Ehdr) *e = (ElfW(Ehdr)*)header;
  ElfW(Shdr) *sh_base = 0, *sh_sym, *sh_str;
  unsigned    sh_base_sz = 0;
  char       *strtab = 0;
  unsigned    strtab_sz = 0;
  int         i;
  void       *scratch_mem;

  for (i=0; i<nr_debug_info; i++)
    if (debug_info[i].tpnt == tpnt)
      {
	if (tpnt != (struct elf_resolve*)~0U)
	  LOGd(DBG, "debug info of object "l4_addr_fmt" already registered",
		     (l4_addr_t)tpnt);
	return;
      }

  if (nr_debug_info >= sizeof(debug_info)/sizeof(debug_info[0]))
    {
      LOGd(DBG, "No free debug_info slot");
      return;
    }

  debug_info[nr_debug_info].symbols_addr = 0;
  debug_info[nr_debug_info].lines_addr_l = 0;
  debug_info[nr_debug_info].tpnt         = tpnt;

  if (!(scratch_mem = _dl_alloc_pages(SCRATCH_SIZE, 0, "tmp scratch")))
    goto error;

  sh_base_sz = e->e_shnum*e->e_shentsize;
  if (!(sh_base = _dl_alloc_pages(sh_base_sz, 0, "tmp secion headers")))
    goto error;
  _dl_seek(infile, e->e_shoff);
  _dl_read(infile, sh_base, e->e_shnum*e->e_shentsize);

  if (e->e_shstrndx != SHN_UNDEF)
    {
      sh_str = sh_base + e->e_shstrndx;
      strtab_sz = sh_str->sh_size;
      if (!(strtab = _dl_alloc_pages(strtab_sz, 0, "tmp strtab")))
	goto error;
      _dl_seek(infile, sh_str->sh_offset);
      _dl_read(infile, (void*)strtab, sh_str->sh_size);
    }

  for (i=0, sh_sym=sh_base; i<e->e_shnum; i++, sh_sym++)
    {
      if (sh_sym->sh_type == SHT_SYMTAB)
	symbols_add(infile, libaddr, scratch_mem, SCRATCH_SIZE, 
		    sh_base, i, debug_info+nr_debug_info);

      if ((sh_sym->sh_type == SHT_PROGBITS || sh_sym->sh_type == SHT_STRTAB) &&
	  strtab && !strcmp(strtab+sh_sym->sh_name, ".stab"))
	lines_add(infile, header, libaddr, scratch_mem, SCRATCH_SIZE,
		  sh_base, i, debug_info+nr_debug_info);
    }

  nr_debug_info++;

error:
  _dl_free_pages(strtab, strtab_sz);
  _dl_free_pages(sh_base, sh_base_sz);
  _dl_free_pages(scratch_mem, SCRATCH_SIZE);
}

void
_dl_debug_info_del(struct elf_resolve *tpnt)
{
  LOGd(DBG, "called");
}

static void
_dl_debug_info_add_bin(void)
{
  char buffer[128];
  const char *fname = binary_name(buffer, sizeof(buffer));
  int infile;
  ElfW(Ehdr) *e;

  if ((infile = _dl_open(fname, O_RDONLY, 0)) < 0)
    {
      LOGd(DBG, "Cannot open binary \"%s\" for symbols", fname);
      return;
    }

  if ((e = _dl_alloc_pages(sizeof(e), 0, "tmp ELF header")))
    {
      _dl_read(infile, e, sizeof(*e));
      _dl_debug_info_add((struct elf_resolve*)~0U, infile, (void*)e, 0);
      _dl_free_pages(e, sizeof(e));
      _dl_close(infile);
    }
}

static void
_dl_debug_info_combine(void)
{
  int i, j;
  char *s, *l;
  l4_addr_t phys;
  l4_threadid_t me = l4_myself();
  l4_size_t sz_l, sz_s;

  _dl_free_pages(glob_symbols, size_symbols);
  _dl_free_pages(glob_lines,   size_lines);

  for (i=0, size_symbols=0, sz_l=0, sz_s=0; i<nr_debug_info; i++)
    {
      size_symbols += debug_info[i].symbols_size;
      sz_l         += debug_info[i].lines_size_l;
      sz_s         += debug_info[i].lines_size_s;
    }

  phys = 0;
  if (size_symbols)
    {
      size_symbols++;

      glob_symbols = _dl_alloc_pages(size_symbols, &phys, 
				     "[SYMBOLS (combined)]");
      if (glob_symbols == (char*)~0U)
	{
	  LOGd(DBG, "Cannot allocate space for combined symbols (need %zdKB)!",
		     size_symbols/1024);
	  return;
	}

      for (i=0, s=glob_symbols; i<nr_debug_info; i++)
	{
	  memcpy(s, debug_info[i].symbols_addr, debug_info[i].symbols_size);
	  s += debug_info[i].symbols_size;
	}

      // terminate list
      *s = '\0';
    }
  fiasco_register_symbols(me, phys, size_symbols);

  phys = 0;
  if (sz_l)
    {
      sz_l += sizeof(stab_line_t);

      size_lines = sz_l+sz_s;
      glob_lines = _dl_alloc_pages(size_lines, &phys,
				   "[LINES (combined)]");
      if (glob_lines == (char*)0U)
	{
	  LOGd(DBG, "Cannot allocate for combined lines (need %zdKB)!",
		    size_lines/1024);
	  return;
	}

      for (i=0, l=glob_lines, s=glob_lines+sz_l; i<nr_debug_info; i++)
	{
	  debug_info_t *d = debug_info+i;
	  memcpy(l, d->lines_addr_l, d->lines_size_l);
	  memcpy(s, d->lines_addr_s, d->lines_size_s);
	  for (j=0; j<d->lines_size_l/sizeof(stab_line_t); j++)
	    {
	      stab_line_t *lin = ((stab_line_t*)l) + j;
	      if (lin->line >= 0xfffd)
      		lin->addr += s - glob_lines;
	    }
	  l += d->lines_size_l;
	  s += d->lines_size_s;
	}

      // terminate list
      memset(l, 0, sizeof(stab_line_t));
    }
  fiasco_register_lines(me, phys, size_lines);
}

void
_dl_debug_info_sum(void)
{
  _dl_debug_info_add_bin();
  _dl_debug_info_combine();
}
