/* $Id$ */
/**
 * \file	exec/server/src/exc_obj_stab.cc
 * \brief	Stub handling code (debugging line information)
 *
 * \date	10/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <l4/sys/kdebug.h>
#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/env/env.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>

#include <stdio.h>
#include <string.h>

#include "exc_obj_stab.h"


exc_obj_stab_t::exc_obj_stab_t(const char *dbg_name)
  : str_field(0), str_end(0), str_max(0), str_ds(L4DM_INVALID_DATASPACE), 
    lin_field(0), lin_end(0), lin_max(0), lin_ds(L4DM_INVALID_DATASPACE),
    name(dbg_name)
{
}

exc_obj_stab_t::~exc_obj_stab_t()
{
  l4dm_mem_release(str_field);
  l4dm_mem_release(lin_field);
}

// realloc function working on dataspace
void*
exc_obj_stab_t::realloc(l4dm_dataspace_t *ds, void *addr, l4_size_t size)
{
  int error;

  size = l4_round_page(size);

  if (size > L4_SUPERPAGESIZE)
    {
      printf("stab size limit of 4MB exceeded\n");
      return 0;
    }

  if (l4dm_is_invalid_ds(*ds))
    {
      l4_threadid_t dsm_id = l4env_get_default_dsm();

      if (l4_is_invalid_id(dsm_id))
	{
	  printf("Can't determine default dataspace manager\n");
	  return 0;
	}

      if ((error = l4dm_mem_open(dsm_id, size, L4_PAGESIZE, 0, name, ds)) < 0)
	{
	  printf("Error %d opening stab ds\n", error);
	  return 0;
	}

      if ((error = l4rm_attach(ds, L4_SUPERPAGESIZE, 0, L4DM_RW, &addr)) < 0)
	{
	  printf("Error %d attaching stab ds\n", error);
	  return 0;
	}
    }
  else
    {
      if ((error = l4dm_mem_resize(ds, size)) < 0)
	{
	  printf("Error %d resizing ds to %08x\n", error, size);
	  return 0;
	}
    }

  return addr;
}

// Search str in str_field. To save space, we search backwards,
// so if bar_foo is already stored, we don't have to store foo
// too
inline int
exc_obj_stab_t::search_str(const char *str, unsigned len, unsigned *idx)
{
  const char *c_next, *c, *d;

  if (!str_field || !*str_field)
    return false;

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
	  return true;	// found (could be a substring)
	}

      // test for end-of-field
      if (!*++c_next)
	return false;	// end of field -- not found
    }
}

inline int
exc_obj_stab_t::add_str(const char *str, unsigned len, unsigned *idx)
{
  // if still included, don't include anymore
  if (search_str(str, len, idx))
    return 0;
  
  // check if str_field is big enough
  if (str_end+len+1 >= str_max)
    {
      char *m;
      
      str_max += 4*L4_PAGESIZE;
      if (!(m = (char*)realloc(&str_ds, str_field, 
			       str_max*sizeof(*str_field))))
	{
	  printf("Can't enlarge str_field to %d kB\n",
	      str_max*sizeof(*str_field)/1024);
	  str_max -= 4*L4_PAGESIZE;
	  return -L4_ENOMEM;
	}
      
      if (!str_field)
	*m = '\0';

      str_field = m;
    }

  *idx = str_end;
  
  // add string to end of str_field
  strcpy(str_field+str_end, str);
  str_end += len+1;

  // terminate string field
  str_field[str_end] = '\0';
  
  return 0;
}

// see documentation in "info stabs"
int
exc_obj_stab_t::add_entry(const stab_entry_t *se, const char *se_str)
{
  const char *se_name = se_str + se->n_strx;
  int error, se_name_len = strlen(se_name);
  unsigned se_name_idx = 0;

  if (se_name_len
      && ((se->n_type == 0x64) || (se->n_type == 0x84)))
    {
      if ((error = add_str(se_name, se_name_len, &se_name_idx)))
	return error;
    }

  if (lin_end >= lin_max)
    {
      stab_line_t *m;
      
      lin_max += 4*L4_PAGESIZE;
      if (!(m = (stab_line_t*)realloc(&lin_ds, lin_field, 
				      lin_max*sizeof(*lin_field))))
	{
	  printf("Can't enlarge lin_field to %d kB\n",
	      lin_max*sizeof(*lin_field)/1024);
	  lin_max -= 4*L4_PAGESIZE;
	  return -L4_ENOMEM;
	}
      lin_field = m;
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

  return 0;
}

int
exc_obj_stab_t::add_section(const stab_entry_t *se,
			    const char *str, unsigned n)
{
  unsigned int i;
  int error;
 
  func_offset = 0;
  have_func   = 0;
  
  for (i=0; i<n; i++)
    if ((error = add_entry(se++, str)))
      return error;

  return 0;
}

int
exc_obj_stab_t::truncate(void)
{
  if (lin_end)
    {
      stab_line_t *m;
      char *c;

      // truncate lines field
      lin_max = lin_end;
      if (!(m = (stab_line_t*)realloc(&lin_ds, lin_field, 
				      lin_max*sizeof(*lin_field))))
	{
	  printf("Can't truncate lin_field to %d bytes\n",
	      lin_max*sizeof(*lin_field));
	  return -L4_ENOMEM;
	}
      lin_field = m;

      // truncate strings field
      str_max = str_end;
      if (!(c = (char*)realloc(&str_ds, str_field, 
			       str_max*sizeof(*str_field))))
	{
	  printf("Can't truncate str_field to %d bytes\n",
	      str_max*sizeof(*str_field));
	  return -L4_ENOMEM;
	}
      
      if (!str_field)
	*c = '\0';

      str_field = c;

      return 0;
    }

  return -L4_EINVAL;
}

int
exc_obj_stab_t::get_size(l4_size_t *str_size, l4_size_t *lin_size)
{
  *str_size = str_end*sizeof(*str_field);
  *lin_size = lin_end*sizeof(*lin_field);
  return 0;
}

int
exc_obj_stab_t::get_lines(char **str, stab_line_t **lin)
{
  memcpy(*str, str_field, str_end*sizeof(*str_field));
  memcpy(*lin, lin_field, lin_end*sizeof(*lin_field));
          *str  += str_end*sizeof(*str_field);
  (char*&)(*lin) += lin_end*sizeof(*lin_field);
  return 0;
}

int
exc_obj_stab_t::print(void)
{
  stab_line_t *l;
  unsigned i, func = 0;
  const char *dir = 0, *fname = 0;

  if (!lin_field)
    return -L4_EINVAL;
 
  printf("str_field %p(%d), lin_field %p(%d)\n", 
      str_field, str_end, lin_field, lin_end);

  for (i=0, l=lin_field; l<lin_field + lin_end; l++)
    {
      unsigned addr = l->addr;
      unsigned line = l->line;
      
      if (line == 0xfffe)
	dir = str_field + addr;
      else if (line == 0xffff || line == 0xfffd)
	fname = str_field + addr;
      else
	{
	  printf("%08x - %s=%s:%d\n", addr+func, dir, fname, line);
      	  i++;
	}
      if (i==22)
	{
	  enter_kdebug("stop");
	  i=0;
	}
    }

  return 0;
}

