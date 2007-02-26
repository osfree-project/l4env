/* $Id$ */
/**
 * \file   l4env/lib/src/startup.c  
 * \brief  Task startup (static version).
 *
 * \date   09/11/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *         Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * L4Env task startup code, basic OSKit 1.0 support.
 * Version for static linking (used by tftp, exec, loader, ...) */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* standard includes */
#include <stdio.h>
#include <stdlib.h>

/* L4 includes */
#include <l4/crtx/crt0.h>
#include <l4/env/env.h>
#include <l4/l4rm/l4rm.h>
#include <l4/semaphore/semaphore.h>
#include <l4/sys/consts.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>
#include <l4/util/util.h>
#include <l4/util/mbi_argv.h>

/* binary test/data segments from linker script */
extern char _stext;
extern char _end;

/* exported symbols */
l4util_mb_info_t *l4env_multiboot_info;

/* list of reserved VM regions (binary segments, boot modules, ...) */
#define MAX_FIXED  32
static l4rm_vm_range_t fixed[MAX_FIXED];
static int num_fixed = 0;

extern int main(int argc, char *argv[]);

/**
 * \brief Application worker thread.
 * Initializes memory and then starts up the application.
 */
static void
__startup_main(void)
{
  /* call constructors, register destructors */
  crt0_construction();

  /* call application */
  exit(main(l4util_argc, l4util_argv));
}

/**
 * \brief Setup fixed VM regions.
 *
 * Reserve fixed program sections we know about (text, data, bss...)
 */
static void
__setup_fixed(void)
{
  /* program text and data sections */
  fixed[num_fixed].addr = l4_trunc_page(&_stext);
  fixed[num_fixed].size = l4_round_page(&_end) - fixed[num_fixed].addr;
  num_fixed++;

  /* RMGR trampoline page */
  fixed[num_fixed].addr = crt0_tramppage & L4_PAGEMASK;
  fixed[num_fixed].size = L4_PAGESIZE;
  num_fixed++;

  /* adapter area (BIOS, graphics memory) */
  fixed[num_fixed].addr = 0xA0000;
  fixed[num_fixed].size = 0x100000 - 0xA0000;
  num_fixed++;

  /* areas where our modules (e.g. files for memfs) live */
  if (l4env_multiboot_info->flags & L4UTIL_MB_MODS)
    {
      l4_umword_t i;
      l4util_mb_mod_t *m;

      m = (l4util_mb_mod_t*) (l4env_multiboot_info->mods_addr);

      for (i=0; i<l4env_multiboot_info->mods_count; i++,m++)
	{
	  l4_addr_t addr     = l4_trunc_page(m->mod_start);
	  l4_addr_t end_addr = l4_round_page(m->mod_end);
	  l4_addr_t size     = end_addr - addr;

	  if (num_fixed == MAX_FIXED)
	    {
	      printf("Startup: too many modules!\n");
	      enter_kdebug("PANIC");
	      return;
	    }

	  fixed[num_fixed].addr = addr;
	  fixed[num_fixed].size = size;
	  num_fixed++;

	  /* page in module */
	  l4_touch_ro((void*)addr, size);
	}
    }
}

/**
 * \brief Task startup.
 */
void __main(void);

void
__main(void)
{
  int ret,i;
  l4_uint32_t area;
  l4_umword_t dummy;
  l4_threadid_t preempter,pager;

  /* parse command line and initialize l4util_argc/l4util_argv */
  l4util_mbi_to_argv(crt0_multiboot_flag, crt0_multiboot_info);

  /* set pointer to multiboot info */  
  l4env_multiboot_info = (l4util_mb_info_t*) crt0_multiboot_info;

  /* init some internal L4env stuff */
  preempter = pager = L4_INVALID_ID;
  l4_thread_ex_regs(l4_myself(), (l4_umword_t)-1, (l4_umword_t)-1,
		    &preempter, &pager, &dummy, &dummy, &dummy);
  l4env_set_sigma0_id(pager);

  /* setup fixed VM areas */
  __setup_fixed();

  /* init region mapper, enable L4ENV */
  if ((ret = l4rm_init(1,fixed,num_fixed)) < 0)
    {
      printf("Startup: setup region mapper failed (%d)!\n",ret);
      enter_kdebug("PANIC");
    }

  /* reserve VM areas */
  for (i = 0; i < num_fixed; i++)
    {
      if ((ret = l4rm_direct_area_reserve_region(fixed[i].addr,fixed[i].size,
						 L4RM_RESERVE_USED,&area)) < 0)
	{
	  printf("Startup: reserve region 0x%08x-0x%08x failed (%d)!\n",
		 fixed[i].addr,fixed[i].addr + fixed[i].size,ret);
	  l4rm_show_region_list();
	  enter_kdebug("PANIC");
	}
    }

  /* init thread lib */
  l4thread_init();

  /* setup region mapper tcb */
  if ((ret = l4thread_setup(l4_myself(),(l4_addr_t)&crt0_stack_low,
					(l4_addr_t)&crt0_stack_high)) < 0)
    {
      printf("Startup: setup region mapper tcb failed (%d)!\n",ret);
      enter_kdebug("PANIC");
    }

  /* init semaphore lib */
  if ((ret = l4semaphore_init()) < 0)
    {
      printf("Startup: semaphore lib initialization failed (%d)!\n",ret);
      enter_kdebug("PANIC");
    }

  /* start main thread */
  if ((ret = l4thread_create((l4thread_fn_t)__startup_main, NULL,
			   L4THREAD_CREATE_ASYNC | L4THREAD_CREATE_SETUP)) < 0)
    {
      printf("Startup: create main thread failed (%d)!\n",ret);
      enter_kdebug("PANIC");
    }

  /* start service loop */
  l4rm_service_loop();
}

/**
 * \brief  Return pointer to L4 environment page
 *	
 * \return NULL
 *
 * Since this is a sigma0-style L4 task, it has no L4 environment infopage
 * mapped from the L4 Loader. But nevertheless, it could be started by the
 * compatibility mode of the L4 loader.
 */
l4env_infopage_t *
l4env_get_infopage(void)
{
  /* deliver 0 since we have nothing to deliver */
  return NULL;
}

