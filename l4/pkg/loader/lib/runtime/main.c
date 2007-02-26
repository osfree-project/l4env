/* $Id$ */
/**
 * \file	loader/lib/src/main.c
 * \brief	Loader library
 *
 * \date	08/22/2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/crtx/crt0.h>
#include <l4/env/env.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>
#include <l4/exec/exec.h>
#include <l4/loader/loader.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/util/mb_info.h>
#include <l4/util/mbi_argv.h>

l4env_infopage_t* app_envpage = NULL;

#define MAX_FIXED 32
static l4rm_vm_range_t fixed[MAX_FIXED];
static int num_fixed = 0;

l4env_infopage_t *
l4env_get_infopage(void)
{
  return app_envpage;
}

// #define DEBUG_SECTIONS

/** print error message and go sleeing */
static void __attribute__((noreturn))
__load_error(const char *format, ...)
{
  va_list args;
  l4_umword_t dummy;
  l4_msgdope_t result;
  
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  /* send answer to loader server */
  l4_ipc_send(app_envpage->loader_id,
		   L4_IPC_SHORT_MSG, L4_LOADER_ERROR, 0,
		   L4_IPC_NEVER, &result);

  /* sleep forever */
  for (;;)
    l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG, &dummy, &dummy, 
		        L4_IPC_NEVER, &result);
}

/** Helper function to extract command line parameters from multiboot info
 * and to start main then */
extern int main(int argc, char *argv[]);

static void
__startup_main(void)
{
  crt0_construction();

  exit(main(l4util_argc, l4util_argv));
}

static void
__setup_fixed(void)
{
  l4exec_section_t *l4exc;
  l4exec_section_t *l4exc_stop;
  
  /* L4env infopage */
  fixed[num_fixed].addr = l4_trunc_page(app_envpage);
  fixed[num_fixed].size = L4_PAGESIZE;
  num_fixed++;

  /* trampoline page */
  fixed[num_fixed].addr = l4_trunc_page(app_envpage->stack_low);
  fixed[num_fixed].size = l4_round_page(app_envpage->stack_high)
			- fixed[num_fixed].addr;
  num_fixed++;

  /* system area (video memory, BIOS */
  fixed[num_fixed].addr = 0x0009F000;
  fixed[num_fixed].size = 0x00100000 - 0x0009F000;
  num_fixed++;

  /* sections which can't be relocated */
  l4exc_stop = app_envpage->section + app_envpage->section_num;
  for (l4exc=app_envpage->section; l4exc<l4exc_stop; l4exc++)
    {
      /* Has the section to be relocated? */
      if (!(l4exc->info.type & L4_DSTYPE_RELOCME))
	{
	  /* No => this section is not relocatable */
	  fixed[num_fixed].addr = l4_trunc_page(l4exc->addr);
	  fixed[num_fixed].size = l4_round_page(l4exc->addr + l4exc->size)
				- fixed[num_fixed].addr;
	  num_fixed++;
	}
    }
}

/** attach all sections with known addresses */
static void
__attach_fixed(void)
{
  int error;
  l4_uint32_t area;
  l4_addr_t addr;
  l4_size_t size;
  l4exec_section_t *l4exc;
  l4exec_section_t *l4exc_stop;

  /* fixed region 0 (see __setup_fixed()) */
  addr = l4_trunc_page(app_envpage);
  size = L4_PAGESIZE;
  if ((error=l4rm_direct_area_reserve_region(addr, size, 0, &area)))
    {
      __load_error("Error %d reserving infopage at %08x-%08x\n",
		    error, addr, addr+size);
    }
  
  /* fixed region 1 (see __setup_fixed()) */
  addr = l4_trunc_page(app_envpage->stack_low);
  size = l4_round_page(app_envpage->stack_high) - addr;
  if ((error=l4rm_direct_area_reserve_region(addr, size, 0, &area)))
    {
      __load_error("Error %d reserving main stack at %08x-%08x\n",
		    error, addr, addr+size);
    }

  /* fixed region 2 (see __setup_fixed()) */
  addr = 0x0009F000;
  size = 0x00100000 - 0x0009F000;
  if ((error=l4rm_direct_area_reserve_region(addr, size, 0, &area)))
    {
      __load_error("Error %d reserving video memory at %08x-%08x\n",
	  error, addr, addr+size);
    }
	
  /* sections which can't be relocated */
  l4exc_stop = app_envpage->section + app_envpage->section_num;
  for (l4exc=app_envpage->section; l4exc<l4exc_stop; l4exc++)
    {
      /* Has the section to be relocated? */
      if (!(l4exc->info.type & L4_DSTYPE_RELOCME))
	{
	  /* No => this section is not relocatable */
      	  addr = l4_trunc_page(l4exc->addr);
	  size = l4_round_page(l4exc->addr+l4exc->size) - addr;
	  
	  if (l4exc->info.type & L4_DSTYPE_PAGEME)
	    {
	      /* section should be paged by our region manager */
	      l4_uint32_t flags = l4exc->info.type & L4_DSTYPE_WRITE 
			     ? L4DM_RW : L4DM_RO;

#ifdef DEBUG_ATTACH
	      printf("attaching fixed section %d to %08x-%08x manager %x.%x\n",
		  l4exc-app_envpage->section, addr, addr+size,
		  l4exc->ds.manager.id.task, l4exc->ds.manager.id.lthread);
#endif
	  
	      if ((error=l4rm_direct_attach_to_region(&l4exc->ds, (void*)addr, 
						      size, 0, flags)))
		{
		  __load_error("Error %d attaching section %d to %08x-%08x\n",
				error, addr, addr+size);
		}

	      /* section is attached now and will be paged */
	      l4exc->info.type &= ~L4_DSTYPE_PAGEME;
	      /* section is known to our region manager */
	      l4exc->info.type &= ~L4_DSTYPE_RESERVEME;
	    }
	  else if (l4exc->info.type & L4_DSTYPE_RESERVEME)
	    {
	      /* section will be paged by someone, only reserve region */
#ifdef DEBUG_ATTACH
	      printf("reserving fixed section %d to %08x-%08x manager %x.%x\n", 
		  l4exc-app_envpage->section, addr, addr+size,
		  l4exc->ds.manager.id.task, l4exc->ds.manager.id.lthread);
#endif
	  
	      if ((error=l4rm_direct_area_reserve_region(addr, size, 0, &area)))
		{
		  __load_error("Error %d reserving section %d to %08x-%08x\n",
				error, addr, addr+size);
		}

	      /* section is reserved now */
	      l4exc->info.type &= ~L4_DSTYPE_RESERVEME;
	    }
	}
    }
}

/** Attach all initial sections which are not yet relocated.
 * 
 * At this time, the region mapper thread is not started yet! */
static void
__attach_relocateable(void)
{
  int error;
  l4_uint32_t area;
  l4_uint32_t flags;
  l4_addr_t sec_beg, sec_end;
  l4_addr_t area_beg, area_end;
  l4_addr_t sec_addr, area_addr;
  l4_size_t sec_size, area_size;
  l4exec_section_t *l4exc;
  l4exec_section_t *l4exc_tmp;
  l4exec_section_t *l4exc_stop;

  /* Go through all sections of the L4 environment infopage and attach
   * sections which are not attched yet. */
  l4exc_stop = app_envpage->section + app_envpage->section_num;
  for (l4exc=app_envpage->section; l4exc<l4exc_stop; l4exc++)
    {
      /* Has the section still to be attached? */
      if (l4exc->info.type & L4_DSTYPE_PAGEME)
	{
	  /* At least this section must be relocated. We can decide, where
	   * it should lay in our address space. With one restriction: The
	   * relative position of sections of the same area must be the
	   * same as in the file image. Therefore we first reserve an free
	   * area for all sections of this ELF object (same area ID) and
	   * then attach all sections to this area */
	  area_beg = L4_MAX_ADDRESS;
	  area_end = 0;

	  for (l4exc_tmp=l4exc; l4exc<l4exc_stop; l4exc++)
	    {
	      sec_beg = l4exc->addr;
	      if (sec_beg < area_beg)
		area_beg = sec_beg;

	      sec_end = l4exc->size + sec_beg;
	      if (sec_end > area_end) 
		area_end = sec_end;
	      
	      if (l4exc->info.type & L4_DSTYPE_OBJ_END)
		break;
	    }
	 
	  /* align area address and area size */
	  area_beg = l4_trunc_page(area_beg);
	  area_end = l4_round_page(area_end);
	  area_size = area_end - area_beg;
	  
	  if (area_beg != 0)
	    __load_error("Error: Relocatable area starts at %08x\n"
			 "sections at %p size %08x\n",
			 area_beg, app_envpage->section, 
			 sizeof(l4exec_section_t));

	  /* reserve area */
	  if ((error = l4rm_direct_area_reserve(area_size, 0,
						&area_addr, &area)))
	    {
	      __load_error("Error reserving area (%08x size %08x)\n",
			   area_addr, area_size);
	    }

	  /* Now as we know the size of the area we attach to it */
	  for (l4exc=l4exc_tmp; l4exc<l4exc_stop; l4exc++)
	    {
	      /* align section address and section size */
	      sec_beg = l4_trunc_page(l4exc->addr);
	      sec_end = l4_round_page(l4exc->addr + l4exc->size);
	      sec_addr = sec_beg;
	      sec_size = sec_end - sec_beg;

	      /* section offsets are relative to area offset */
	      sec_addr += area_addr;

	      flags = l4exc->info.type & L4_DSTYPE_WRITE ? L4DM_RW : L4DM_RO;
	    
#ifdef DEBUG_ATTACH
	      printf("attaching reloc section %d to %08x-%08x manager %x.%x\n", 
		  l4exc-app_envpage->section, sec_addr, sec_addr+sec_size,
		  l4exc->ds.manager.id.task, l4exc->ds.manager.id.lthread);
#endif

	      /* Sections have fixed offsets in the region */
	      if ((error = l4rm_direct_area_attach_to_region(&l4exc->ds,
				area, (void *)sec_addr, sec_size, 0, flags)))
		{
		  __load_error("Error %d attaching to area id %d "
			       "(%08x size %08x)\n",
			       error, area, sec_addr, sec_addr+sec_size);
		}

	      l4exc->addr = sec_addr;
	      l4exc->info.type &= ~L4_DSTYPE_RELOCME;
	      l4exc->info.type &= ~L4_DSTYPE_PAGEME;
	      l4exc->info.type &= ~L4_DSTYPE_RESERVEME;

	      if (l4exc->info.type & L4_DSTYPE_OBJ_END)
		break;
	    }
	}
    }
}

static void
__fixup_modules(void)
{
  l4util_mb_info_t *mbi = (l4util_mb_info_t*)(app_envpage->addr_mb_info);

  if (mbi->flags & L4UTIL_MB_MODS)
    {
      int i;
      l4util_mb_mod_t *mods = (l4util_mb_mod_t*)mbi->mods_addr;

      for (i=0; i<mbi->mods_count; i++)
	{
	  if (   (mods->mod_start < L4ENV_MAXSECT)
	      && (mods->mod_end   == 0))
	    {
	      l4exec_section_t *l4exc = app_envpage->section + mods->mod_start;
	      
	      if (l4exc->info.type & L4_DSTYPE_RELOCME)
		{
		  __load_error("Error: Module not relocated\n");
		}
	      
	      mods->mod_start = l4exc->addr;
	      mods->mod_end   = l4exc->addr + l4exc->size;
	    }
	}
    }
}

/**  Shake hands with loader server.
 *
 * After all sections are attached to our region mapper, we ask the loader to
 * relocate/link the remaining sections/ELF objects.
 *
 * \return		address of real program entry point */
static l4_addr_t
__complete_load(void)
{
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  
  l4_ipc_call(app_envpage->loader_id,
		   L4_IPC_SHORT_MSG, L4_LOADER_COMPLETE, 0,
		   L4_IPC_SHORT_MSG, &dw0, &dw1,
		   L4_IPC_NEVER, &result);

  if (dw0 != L4_LOADER_COMPLETE)
    __load_error("Got illegal message %08x from loader\n", dw0);

  return (l4_addr_t)app_envpage->entry_2nd;
}

/** First entry point.
 *
 * This function is called by the Loader server. It's task is to attach all
 * regions of the infopage to our address space so that the region mapper can
 * page the sections later (after the region mapper pager thread is started.
 *
 * \param infopage	L4 environment infopage */
void
l4loader_init(void *infopage)
{
  l4_addr_t start_addr;
  
  app_envpage = (l4env_infopage_t*)infopage;

  __setup_fixed();

  /* init data structures of Region Mapper */
  l4rm_init(1, fixed, num_fixed);

  /* don't reserve fixed[] regions here but reserve them in __attach_fixed() */

  /* attach all initial sections (text, data, ...) */
  __attach_fixed();
  __attach_relocateable();
  __fixup_modules();

  /* shake hands with exec layer and complete load process */
  start_addr = __complete_load();

  asm volatile("push %0 ; ret" : :"r"(start_addr));
}

/** Second entry point.
 *
 */
void
l4env_init(void)
{
  int ret;

  /* attach rest of relocated sections which the loader */
  __attach_relocateable();

  /* init command line parameters */
  l4util_mbi_to_argv(L4UTIL_MB_VALID, 
		     (l4util_mb_info_t*)app_envpage->addr_mb_info);

  /* init thread lib */
  l4thread_init();

  /* setup region mapper tcb */
  if ((ret = l4thread_setup(l4_myself(),
			    (l4_addr_t)app_envpage->stack_low,
			    (l4_addr_t)app_envpage->stack_high))<0)
    {
      printf("l4env_init: Setup region mapper tcb failed (%d)\n", ret);
      enter_kdebug("l4env_init");
    }

  /* init semaphore lib */
  if ((ret = l4semaphore_init())<0)
    {
      printf("l4env_init: Setup semaphore lib failed (%d)\n", ret);
      enter_kdebug("l4env_init");
    }

  /* start thread */
  if ((ret = l4thread_create((l4thread_fn_t)__startup_main,
			     (void*)app_envpage->addr_mb_info,
			    L4THREAD_CREATE_ASYNC | 
			    L4THREAD_CREATE_SETUP)) < 0)
    {
      printf("l4env_init: create main thread failed (%d)!\n",ret);
      enter_kdebug("PANIC");
    }

  /* start service loop */
  l4rm_service_loop();
}

void __main(void);

void
__main(void)
{
}

