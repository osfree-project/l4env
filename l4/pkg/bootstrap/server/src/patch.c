/* $Id$ */
/**
 * \file	bootstrap/server/src/patch.c
 * \brief	Patching of boot modules
 * 
 * \date	09/2005
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <l4/sys/consts.h>
#include <l4/util/mb_info.h>
#include <l4/env_support/panic.h>

#ifdef ARCH_x86
#include "ARCH-x86/macros.h"
#endif

#ifdef ARCH_arm
#include "ARCH-arm/macros.h"
#endif

#ifdef ARCH_amd64
#include "ARCH-amd64/macros.h"
#endif

#include "types.h"
#include "patch.h"

static char  argspc[4096];
static char *argptr = argspc;
static char *argarr[MODS_MAX];

/* search module in module list */
static l4util_mb_mod_t*
search_module(const char *name, size_t name_len, l4util_mb_info_t *mbi,
              const char **cmdline, const char **modname_end)
{
  int i;
  const char *c = 0, *ce = 0;
  l4util_mb_mod_t *mod;

  for (i=0; i<mbi->mods_count; i++)
    {
      const char *m, *n;

      mod = (L4_MB_MOD_PTR(mbi->mods_addr)) + i;
      m = c = L4_CONST_CHAR_PTR(mod->cmdline);
      ce = strchr(c, ' ');
      if (!ce)
	ce = c+strlen(c);
      for (;;)
	{
	  if (!(n = strchr(m, name[0])) || n+name_len>ce)
	    break;
	  if (!memcmp(name, n, name_len))
	    {
	      *cmdline = c;
	      return mod;
	    }
	  m = n+1;
	}
    }

  return NULL;
}

/**
 * Handle -patch=<module_name>,<variable>=blah parameter. Overwrite a specific
 * module from command line. This allows to change the boot configuration (e.g.
 * changing parameters of a loader script)
 */
void
patch_module(const char **str, l4util_mb_info_t *mbi)
{
  const char *nam_beg, *nam_end;
  const char *var_beg, *var_end;
  const char *val_beg, *val_end;
  char *mod_beg, *mod_end, *mod_ptr, quote = 0;
  l4_size_t var_size, val_size, max_patch_size;
  const char *cmdline = 0, *modname_end = 0;
  l4util_mb_mod_t *mod;

  /* nam_beg ... nam_end */
  nam_beg = *str+8;
  nam_end = strchr(nam_beg, ',');
  if (!nam_end || strpbrk(nam_beg, "\011 =*")-1 < nam_end)
    panic("-patch: bad module name");

  mod = search_module(nam_beg, nam_end-nam_beg, mbi, &cmdline, &modname_end);
  if (!mod)
    panic("-patch: cannot find module \"%.*s\"", 
	  (int)(nam_end-nam_beg), nam_beg);

  mod_beg = L4_CHAR_PTR(mod->mod_start);
  mod_end = L4_CHAR_PTR(mod->mod_end);

  /* How much bytes the module can be enlarged to. The module cannot
   * be extended beyond page boundaries because the next module starts
   * there and we don't want to move the following modules. */
  max_patch_size = l4_round_page(mod_end) - (l4_addr_t)mod_end - 1;

  printf("  Patching module \"%.*s\"\n",
         (unsigned)(modname_end - cmdline), cmdline);

  for (var_beg=nam_end; *var_beg==','; var_beg=*str)
    {
      var_beg++;
      /* var_beg ... var_end */
      var_end = strchr(var_beg, '=');
      if (!var_end || strpbrk(var_beg, "\011 ,*")-1 < nam_end)
	panic("-patch: bad variable name");
      var_size = var_end-var_beg;

      /* val_beg ... val_end, consider quotes */
      val_beg = val_end = var_end+1;
      if (*val_end == '"' || *val_end == '\'')
	{
	  val_beg++;
	  quote = *val_end++;
	}
      while (*val_end && ((!quote && !isspace(*val_end) && *val_end!=',') ||
			  ( quote && *val_end!=quote)))
	val_end++;
      *str = val_end;
      if (quote)
	(*str)++;
      quote = 0;
      val_size = val_end-val_beg;

      /* replace all occurences of variable with value */
      for (mod_ptr=mod_beg;;)
	{
	  if (!(mod_ptr = memmem(mod_ptr, mod_end-mod_ptr, 
				 var_beg, var_end-var_beg)))
	    break;
	  if (var_size < val_size && max_patch_size < val_size-var_size)
	    panic("-patch: not enough space in module");
	  max_patch_size += var_size - val_size;
	  memmove(mod_ptr+val_size, mod_ptr+var_size, mod_end-mod_ptr-var_size);
	  if (val_size < var_size)
	    memset(mod_end-var_size+val_size, 0, var_size-val_size);
	  memcpy(mod_ptr, val_beg, val_size);
	  mod_ptr += val_size;
	  mod_end += val_size - var_size;
	}
    }

  mod->mod_end = (l4_addr_t)mod_end;
}

/**
 * Handle -cmdline=<module_name>,blah parameter. Replace old command line
 * parameters of <module_name> by blah. Useful for changing the boot
 * configuration of a bootstrap image.
 */
void
args_module(const char **str, l4util_mb_info_t *mbi)
{
  const char *nam_beg, *nam_end;
  const char *val_beg, *val_end;
  char quote = 0;
  l4_size_t val_size;
  const char *cmdline = 0, *modname_end = 0;
  l4util_mb_mod_t *mod;
  int modnr;

  /* nam_beg ... nam_end */
  nam_beg = *str+8;
  nam_end = strchr(nam_beg, ',');
  if (!nam_end || strpbrk(nam_beg, "\011 =*")-1 < nam_end)
    panic("-args: bad module name");

  mod = search_module(nam_beg, nam_end-nam_beg, mbi, &cmdline, &modname_end);
  if (!mod)
    panic("-args: cannot find module \"%.*s\"",
	  (int)(nam_end-nam_beg), nam_beg);

  modnr = mod - L4_MB_MOD_PTR(mbi->mods_addr);
  if (argarr[modnr])
    panic("-args: called more than once for same module, use quotes");

  printf("  Replacing args of module %d: \"%.*s\"\n", 
      modnr, (unsigned)(modname_end - cmdline), cmdline);

  /* val_beg ... val_end, consider quotes */
  val_beg = val_end = nam_end+1;
  if (*val_end == '"' || *val_end == '\'')
    {
      val_beg++;
      quote = *val_end++;
    }
  while (*val_end && ((!quote && !isspace(*val_end) && *val_end!=',') ||
		      ( quote && *val_end!=quote)))
    val_end++;
  *str = val_end;
  if (quote)
    (*str)++;
  quote = 0;
  val_size = val_end-val_beg;

  if (argptr + val_size + 1 > argspc + sizeof(argspc))
    panic("-cmdline: internal parameter memory exceeded");

  argarr[modnr] = memcpy(argptr, val_beg, val_size);
  argptr += val_size;
  *argptr++ = '\0';
}

const char *
get_args_module(int i)
{
  if (i >= MODS_MAX)
    return NULL;

  return argarr[i];
}
