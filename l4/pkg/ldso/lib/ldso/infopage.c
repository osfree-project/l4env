#include <stdio.h>
#include <l4/env/env.h>
#include <l4/sys/kdebug.h>
#include <l4/util/l4_macros.h>
#include <l4/exec/exec.h>

#include "emul_linux.h"
#include "infopage.h"

l4env_infopage_t *global_env;

/** Show all sections listed in L4env infopage */
void
infopage_show_sections(void)
{
  int i;
  l4exec_section_t *l4exc;

  for (i=0, l4exc=global_env->section; i<global_env->section_num; i++, l4exc++)
    {
      printf("  %08x-%08x [%c%c%c] ds %d at "l4util_idfmt"\n",
	  l4exc->addr,
	  l4exc->addr + global_env->section[i].size,
	  l4exc->info.type & L4_DSTYPE_READ    ? 'r' : '-',
	  l4exc->info.type & L4_DSTYPE_WRITE   ? 'w' : '-',
	  l4exc->info.type & L4_DSTYPE_EXECUTE ? 'x' : '-',
	  l4exc->ds.id,
	  l4util_idstr(l4exc->ds.manager));
    }
}

/** Add a reserved entry for the mmap area. */
void
infopage_add_mmap_area(void)
{
  l4exec_section_t *l4exc;

  if (global_env->section_num >= L4ENV_MAXSECT)
    {
      printf("L4ENV_MAXSECT too small\n");
      enter_kdebug("stop");
      return;
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
