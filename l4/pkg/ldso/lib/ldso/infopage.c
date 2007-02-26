#include <stdio.h>
#include <l4/env/env.h>
#include <l4/sys/kdebug.h>
#include <l4/util/l4_macros.h>
#include <l4/exec/exec.h>
#include <l4/log/l4log.h>

#include "emul_linux.h"
#include "infopage.h"
#include "dl-syscall.h"

#if DEBUG_LEVEL>0
#define DBG 1
#else
#define DBG 0
#endif

l4env_infopage_t *global_env;

/** Show all sections listed in L4env infopage */
void
infopage_show_sections(void)
{
#if DEBUG_LEVEL>0
  int i;
  l4exec_section_t *l4exc;

  for (i=0, l4exc=global_env->section; i<global_env->section_num; i++, l4exc++)
    {
      printf("  "l4_addr_fmt"-"l4_addr_fmt" [%c%c%c] ds %4d at "
	  l4util_idfmt"\n",
	  l4exc->addr,
	  l4exc->addr + global_env->section[i].size,
	  l4exc->info.type & L4_DSTYPE_READ    ? 'r' : '-',
	  l4exc->info.type & L4_DSTYPE_WRITE   ? 'w' : '-',
	  l4exc->info.type & L4_DSTYPE_EXECUTE ? 'x' : '-',
	  l4exc->ds.id,
	  l4util_idstr(l4exc->ds.manager));
    }
#endif
}

/** Add a reserved entry for the mmap area. */
void
infopage_add_mmap_area(void)
{
  l4exec_section_t *l4exc;

  if (global_env->section_num >= L4ENV_MAXSECT)
    {
      LOGd(DBG, "L4ENV_MAXSECT too small");
      _dl_exit(1);
    }

  /* look for overlaps */
  for (l4exc = global_env->section;
       l4exc < global_env->section + global_env->section_num;
       l4exc++)
    {
      if (l4exc->addr < MMAP_END && l4exc->addr+l4exc->size > MMAP_START)
	{
	  LOGd(DBG, "mmap area ("l4_addr_fmt"-"l4_addr_fmt") overlaps region %d"
	            " ("l4_addr_fmt"-"l4_addr_fmt")",
		    (l4_addr_t)MMAP_START, (l4_addr_t)MMAP_END,
		    l4exc-global_env->section,
		    l4exc->addr, l4exc->addr+l4exc->size);
	  _dl_exit(1);
	}
    }

  /* mark mmap area as reserved */
  l4exc            = global_env->section + global_env->section_num;
  l4exc->addr      = MMAP_START;
  l4exc->size      = MMAP_END - MMAP_START;
  l4exc->info.type = 0;
  l4exc->info.id   = 0;
  l4exc->ds        = L4DM_INVALID_DATASPACE;
  global_env->section_num++;
}

/** Needed by dm_mem/open.c */
l4_threadid_t
l4env_get_default_dsm(void)
{
  return global_env->memserv_id;
}
