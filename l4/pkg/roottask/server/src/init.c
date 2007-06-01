/* startup stuff */

/* it should be possible to throw away the text/data/bss of the object
   file resulting from this source -- so, we don't define here
   anything we could still use at a later time.  instead, globals are
   defined in globals.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <ctype.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/memdesc.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/rmgr/proto.h>
#if defined ARCH_x86 | defined ARCH_amd64
#include <l4/util/port_io.h>
#endif
#include <l4/sigma0/sigma0.h>

#include "bootquota.h"
#include "memmap.h"
#include "iomap.h"
#include "irq.h"
#include "globals.h"
#include "exec.h"
#include "rmgr.h"
#include "cfg.h"
#include "symbols.h"
#include "lines.h"
#include "init.h"
#include "boot_error.h"
#include "module.h"
#include "version.h"
#include "pager.h"
#include "rmgr.h"
#include "trampoline.h"
#include "names.h"
#include "quota.h"
#include "irq_thread.h"
#include "small.h"
#include "task.h"
#include "region.h"
#include "vm.h"
#include "macros.h"
#include "kinfo.h"

static int verbose;
static int no_symbols;			/* 1=don't load module symbols */
static int no_lines;			/* 1=don't load module stab info */
static int fiasco_symbols;		/* found fiasco symbols */
static int fiasco_lines;		/* found fiasco lines */
static int configfile;			/* found config file */
static int memdump;

       l4util_mb_info_t *mb_ptr;	/* is written by crt */
static l4util_mb_info_t *mb_info;
static l4util_mb_vbe_ctrl_t *mb_vbe_ctrl; /* VESA contr. info  */
static l4util_mb_vbe_mode_t *mb_vbe_mode; /* VESA mode info  */
static l4util_mb_mod_t *mb_mod;		/* multiboot modules  */
static unsigned first_task_module;
static int symbols_module;		/* we have a fiasco symbols module */
static int lines_module;		/* we have a fiasco lines module */
static int config_module;		/* we have a configfile module */
static l4_addr_t mod_range_start;
static l4_addr_t mod_range_end;
static l4_size_t free_size;		/* counter for free memory */
static l4_size_t reserved_size;		/* counter for reserved memory */


static char *config_start;
static char *config_end;

extern void reset_malloc(void);

#ifndef SIGMA0_REQ_MAGIC
#error Update your sigma0/libsigma0
#endif

/*
 * Map the KIP to the given address 'to_addr'
 */
static l4_kernel_info_t *
map_kip(l4_addr_t to_addr)
{
  l4_msgdope_t result;
  l4_snd_fpage_t sfpage;
  int error;
  l4_kernel_info_t *k = (l4_kernel_info_t *)to_addr;

  l4_fpage_unmap(l4_fpage((l4_umword_t)k, L4_LOG2_PAGESIZE, 0, 0),
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);

  error = l4_ipc_call(my_pager,
		      L4_IPC_SHORT_MSG, SIGMA0_REQ_KIP, 0,
		      L4_IPC_MAPMSG((l4_umword_t)k, L4_LOG2_PAGESIZE),
			&sfpage.snd_base, &sfpage.fpage.fpage,
		      L4_IPC_NEVER, &result);

  if (error)
    boot_panic("can't map KIP: IPC error 0x%02x", error);

  if (k->magic != L4_KERNEL_INFO_MAGIC)
    boot_panic("invalid KIP magic %08x", k->magic);

  return k;
}

/* find the corresponding module for a name */
void
find_module(int *cfg_mod, char* cfg_name)
{
  char *n, *space, *found;
  int i;

  for (i = first_task_module; i < mb_info->mods_count; i++)
    {
      if (!(n = L4_CHAR_PTR mb_mod[i].cmdline))
	{
	  boot_warning("cmdlines unsupported, can't find modname %s\n", 
		       cfg_name);
	  *cfg_mod = -1;
	  return;
	}

      space = strchr(n, ' ');
      if (space)
	*space = '\0';        /* don't search in arguments */
      if (strchr(n, '/'))     /* don't search in path */
	{
	  char *fname = strrchr(n, '/');
	  found = strstr(fname, cfg_name);
	}
      else
	found = strstr(n, cfg_name);
      if (space)
	*space = ' ';        /* restore string */

      if (found)
	{
	  *cfg_mod = i;
	  return;
	}
    }

  // we are configuring not a boot module
  *cfg_mod = -1;
  return;
}

/* check if the module has the name so we can configure it */
void
check_module(unsigned cfg_mod, char *cfg_name)
{
  char *n;

  if (cfg_mod < 1)
    boot_error("no boot module associated with modname %s", cfg_name);

  else if (!(n = L4_CHAR_PTR mb_mod[cfg_mod].cmdline))
    boot_error("cmdlines unsupported, can't verify modname %s", cfg_name);

  else if (!strstr(n, cfg_name))
    boot_error("cmdline %s doesn't match modname %s", n, cfg_name);
}

/**
 * free memory occupied by module image as loaded by GRUB
 */
static void
free_module_image(unsigned mod_no)
{
  l4_addr_t beg, end, addr;

  /* if mod_start does not start at page boundary, skip first page */
  beg = l4_round_page(mb_mod[mod_no].mod_start);
  end = l4_round_page(mb_mod[mod_no].mod_end);

  /* free the memory that module occupied */
  for (addr = beg; addr < end; addr += L4_PAGESIZE)
    memmap_free_page(addr, O_RESERVED);

  /* for sanity/security */
  memset((void*)beg, 0, end-beg);

  region_free(beg, end);
}

static void
configure(void)
{
  if (configfile)
    {
      char *start, *end;

      start = L4_CHAR_PTR mb_mod[config_module].mod_start;
      end   = L4_CHAR_PTR mb_mod[config_module].mod_end;

      if (strncmp(start, "#!rmgr", 6)
          && strncmp(start, "#!roottask", 10))
	{
	  boot_error("config file doesn't start with \"#!roottask\")");
	  return;
	}

      cfg_init();
      cfg_setup_input(start, end);
      puts("\nRoottask: Parsing config file.");
      if (cfg_parse())
	boot_error("Parse error in config file");

      free_module_image(config_module);
    }

  // check also for cmdline configuration
  if (!config_start)
    return;

  puts("\nRoottask: Parsing command line config.");

  cfg_init();
  cfg_setup_input(config_start, config_end);

  if (cfg_parse())
    boot_error("failed parsing configuration command line.");

  // don't do this between the two cfg_parse() runs ...
  reset_malloc();

  /* a few post-parsing actions follow... */
  if (small_space_size)
    {
      l4_addr_t s = small_space_size;

      small_space_size /= 0x400000; /* divide by 4MB */
      if (s > small_space_size * 0x400000 /* did we delete any size bits? */
	  || (small_space_size & (small_space_size - 1))) /* > 1 bit set? */
	{
	  /* use next largest legal size */
	  int count;
	  for (count = 0; small_space_size; count++)
	    small_space_size >>= 1;

	  small_space_size = 1L << count;
	}

      if (small_space_size > 64)
	{
	  small_space_size = 0;
	  boot_error(" small_space_size 0x"l4_addr_fmt" too large", s);
	}
      else
	{
	  unsigned i;

	  /* XXX make check for small_free  XXX */
	  for (i = 1; i < 128/small_space_size; i++)
	    small_free(i, O_RESERVED);

	  printf("  configured small_space_size = %d MB\n",
		 small_space_size * 4);
	}
    }
}

/**
 * setup the small address space of the task.
 */
static int
set_small(l4_threadid_t t, int num)
{
  l4_sched_param_t sched;
  l4_threadid_t s;

  if (num < RMGR_SMALL_MAX && small_alloc(num, t.id.task))
    {
      s = L4_INVALID_ID;
      l4_thread_schedule(t, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
      sched.sp.small = small_space_size | (num * small_space_size * 2);
      s = L4_INVALID_ID;
      l4_thread_schedule(t, sched, &s, &s, &sched);
    }
  else
    {
      boot_error("can't set small space %d for task 0x%x",
	     num, t.id.task);
      return 0;
    }

  return 1;
}

/**
 * setup the priority of the task.
 */
static int
set_prio(l4_threadid_t t, int num)
{
  l4_sched_param_t sched;
  l4_threadid_t s;

  s = L4_INVALID_ID;
  l4_thread_schedule(t, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
  sched.sp.prio = num;
  /* don't mess up anything else than the thread's priority */
  sched.sp.state = 0;
  sched.sp.small = 0;
  s = L4_INVALID_ID;
  l4_thread_schedule(t, sched, &s, &s, &sched);

  return 1;
}


/**
 * some task specific setup.
 */
static void
setup_task(l4_threadid_t t)
{
  bootquota_t *b = bootquota_get(t.id.task);

  if (b->small_space != 0xff)
    {
      if (small_space_size)
	set_small(t, b->small_space);
      else
	b->small_space = 0xFF;
    }

  set_prio(t, b->prio);
}

/**
 * set configuration to default values and parse command line.
 */
static void
init_config(void)
{
  /* init multiboot info structures provided by bootstrap */
  kip = map_kip(0x1000);

  if (kip->version >> 24 == 0x87)
    l4_version = VERSION_FIASCO;

  if (kip->user_ptr)
    mb_info = (l4util_mb_info_t*)kip->user_ptr;
  else
    boot_panic("no multiboot info found");

  mb_vbe_ctrl  = L4_MB_VBE_CTRL_PTR mb_info->vbe_ctrl_info;
  mb_vbe_mode  = L4_MB_VBE_MODE_PTR mb_info->vbe_mode_info;
  mb_mod       = L4_MB_MOD_PTR mb_info->mods_addr;

  /* set mem size */
#if defined (ARCH_x86) || defined(ARCH_amd64)
  mem_lower = mb_info->mem_lower & ~3; // round down to next 4k boundary
#endif
  mem_high  = root_kinfo_mem_high()-RAM_BASE;
  mem_high  = l4_trunc_page(mem_high); // round down to next 4k boundary

  if (mem_lower > 0x9F000 >> 10)
    mem_lower = 0x9F000 >> 10;
  if (MEM_MAX && mem_high > MEM_MAX)
    mem_high = MEM_MAX;

  quiet = 0;
  verbose = 0;
  first_task_module = 3; // 0 is fiasco, 1 is sigma0, 2 is roottask

  char *cmdl = (char*)mb_mod[first_task_module-1].cmdline;

  if (1 || verbose)
    printf("  Command line found: \"%s\"\n", cmdl);

  /* skip module name */
  while (*cmdl && !isspace(*cmdl))
    cmdl++;

  while (*cmdl)
    {
      while (isspace(*cmdl))
	cmdl++;

      if (strstr(cmdl, "-quiet") == cmdl)
	quiet = 1;
      else if (strstr(cmdl, "-verbose") == cmdl)
	verbose = 1;
      else if (strstr(cmdl, "-memdump") == cmdl)
	memdump = 1;
      /* give debug information */
      else if (strstr(cmdl, "-nopentium") == cmdl)
	no_pentium = 1;
      /* don't extract  symbols information from modules */
      else if (strstr(cmdl, "-nosymbols") == cmdl)
	no_symbols = 1;
      /* don't extract lines information from modules */
      else if (strstr(cmdl, "-nolines") == cmdl)
	no_lines = 1;
      /* we got fiasco symbols as module */
      else if (strstr(cmdl, "-symbols") == cmdl)
	{
	  fiasco_symbols = 1;
	  symbols_module = first_task_module++;
	}
      /* we got fiasco lines as module */
      else if (strstr(cmdl, "-lines") == cmdl)
	{
	  fiasco_lines = 1;
	  lines_module = first_task_module++;
	}
      /*  we got config as loaded module */
      else if (strstr(cmdl, "-configfile") == cmdl)
	{
	  configfile = 1;
	  config_module = first_task_module++;
	}
      else if (strstr(cmdl, "-- ") == cmdl)
	{
	  cmdl += 3;
	  break; /* '--' means end of parameter processing */
	}
      else if (*cmdl != '-')
	break; /* Non parameter follows -> configuration data */

      /* Advance to next whitespace */
      while (*cmdl && !isspace(*cmdl))
	cmdl++;
    }

  /* Everything else is configuration */
  if (*cmdl)
    {
      config_start = cmdl;
      config_end = config_start + strlen(config_start);
      if (verbose)
	printf("  Cmdline configuration: \"%s\"\n", cmdl);
    }

  /* first_task_module now points to the first loadable module */
}

/**
 * initialize rmgr specific variables.
 */
static void
init_rmgr(void)
{
  l4_umword_t dummy;

  /* set myself (my thread id) */
  myself = rmgr_pager_id = rmgr_super_id = l4_myself();
  rmgr_pager_id.id.lthread = RMGR_LTHREAD_PAGER;
  rmgr_super_id.id.lthread = RMGR_LTHREAD_SUPER;

  /* set my_pager */
  my_preempter = my_pager = L4_INVALID_ID;
  l4_thread_ex_regs(myself, (l4_umword_t) -1, (l4_umword_t) -1,
		    &my_preempter, &my_pager, &dummy, &dummy, &dummy);

  debug_log_mask = 0;
  debug_log_types = 0;
  small_space_size = 0;
}

/**
 * initialize quota handling.
 */
static void
init_quota(void)
{
  bootquota_init();
  quota_init();
}

/**
 * start for every irq a seperate Irq thread.
 */
static void
init_irq(void)
{
#if defined(ARCH_x86) | defined(ARCH_amd64)
  int i;
  l4_umword_t code;

  /* initialize to "reserved" */
  irq_init();

  printf("  Attached irqs = [ ");

  for (i = 0; i < RMGR_IRQ_MAX; i++)
    {
      code = irq_get(i);

      if (ux_running)
	{
	  if (code == 0)
	    {
	      irq_free(i, O_RESERVED);
	      printf("%X ", i);
	    }
	  else
	    printf("<!%X> ", i);
	}
      else
	{
	  if (i == 2)
	    {
	      /* enable IRQ2 */
	      unsigned char c;
	      c = l4util_in8(0x21);
	      l4util_iodelay();
	      l4util_out8(c & 0xfb, 0x21);
	    }

	  if (code == 0)
	    {
	      if (i != 2)
		irq_free(i, O_RESERVED); /* free irq, except if irq2 */
	      printf("%X ", i);
	    }
	  else
	    printf("<!%X> ", i);
	}
    }
  printf("]\n");
#endif
}

/**
 * initialize task management from rmgr.
 */
static void
init_task(void)
{
  task_init();
  task_set(myself.id.task, RMGR_TASK_MAX, O_FREE);
}

/**
 * init small address spaces.
 */
static void
init_small(void)
{
  small_init();
}

/**
 * get as much super (4MB) pages as possible, starting with
 * superpage 1 (0x400000).
 */
static void
pagein_4MB_memory(void)
{
  l4_addr_t address;
  l4_size_t size;
  l4_msgdope_t result;
  l4_snd_fpage_t sfpage;
  int error;

  for (address = ram_base
#ifdef ARCH_x86
		 + L4_SUPERPAGESIZE
#endif

#ifdef ARCH_amd64
		 + 2*L4_SUPERPAGESIZE
#endif
       ;
       MEM_MAX ? address - ram_base < MEM_MAX : address != MEM_MAX;
       address += L4_SUPERPAGESIZE)
    {
      // new
      error = l4_ipc_call(my_pager,
			  L4_IPC_SHORT_MSG, SIGMA0_REQ_FPAGE_RAM,
			    l4_fpage(address, 
				     (address & (L4_SUPERPAGESIZE-1))
					? L4_LOG2_PAGESIZE
					: L4_LOG2_SUPERPAGESIZE,
				      0, 0).fpage,
			  L4_IPC_MAPMSG(0, L4_WHOLE_ADDRESS_SPACE),
			    &sfpage.snd_base, &sfpage.fpage.fpage,
	    		  L4_IPC_NEVER, &result);
      /* XXX should we check for errors? */

      if (!l4_ipc_fpage_received(result))
	continue;

      if (!(l4_ipc_is_fpage_writable(sfpage.fpage)))
	  boot_panic("received not writable flexpage at %x", address);

      /* mem_high must be set here since memmap_free_page() depends on it */
      size = 1L << sfpage.fpage.fp.size;
      if (mem_high + ram_base < address + size)
	mem_high = address + size - ram_base;

      if (!(sfpage.fpage.fp.size == L4_LOG2_SUPERPAGESIZE))
	{
	  /* we've not received a super page */
	  l4_addr_t a;

	  for (a = address; size > 0; size -= L4_PAGESIZE, a += L4_PAGESIZE)
	    memmap_free_page(a, O_RESERVED);

	  free_size += size;
	}
      else
	{
	  /* we've received a super page */
	  l4_addr_t a;

	  for (a = address; a < address + L4_SUPERPAGESIZE; a += L4_PAGESIZE)
	    memmap_free_page(a, O_RESERVED);

	  free_size += L4_SUPERPAGESIZE;

	  assert(memmap_owner_superpage(address) == O_FREE);
	}
    }
}



/**
 * get as much 4KB pages as possible.
 */
static void
pagein_4KB_memory(void)
{
  l4_addr_t base;

  for (;;)
    {
      switch (l4sigma0_map_anypage(my_pager, 0, L4_WHOLE_ADDRESS_SPACE, &base))
	{
	case -2:
	  boot_panic("can't map memory pages: (IPC error)");
	  return;

	case -3:
	  /* no more pages from sigma0 */
	  return;

	}
      if (MEM_MAX && base >= MEM_MAX + (unsigned long)ram_base)
	/* cannot handle this page */
	continue;

      /* set mem_high to new value */
      if (mem_high < base - ram_base + L4_PAGESIZE)
	mem_high = base - ram_base + L4_PAGESIZE;

      if (memmap_owner_page(base) == O_FREE)
	{
	  boot_warning("got page 0x"l4_addr_fmt" twice!\n", base);
	  continue;
	}

      memmap_free_page(base, O_RESERVED);
      free_size += L4_PAGESIZE;
    }
}

#if defined ARCH_x86 | ARCH_amd64
/**
 * page in BIOS data area page explicitly.  the BIOS area will be
 * marked "reserved" and special-cased in the pager() in pager.c
 */
static void
pagein_bios_memory(void)
{
  l4_msgdope_t result;
  l4_snd_fpage_t sfpage;
  int error;

  error = l4_ipc_call(my_pager, L4_IPC_SHORT_MSG,
		      0, 0,
		      L4_IPC_MAPMSG(0, L4_WHOLE_ADDRESS_SPACE),
		      &sfpage.snd_base, &sfpage.fpage.fpage,
		      L4_IPC_NEVER, &result);

  if (error)
    boot_panic("can't map BIOS area (IPC error 0x%02x)", error);

  region_add(sfpage.snd_base, 1U << sfpage.fpage.fp.size, -1, "BIOS area");
}

/**
 * page in adapter space explicitly.  the adapter space will be
 * marked "reserved" and special-cased in the pager() in memmap.c
 */
static void
pagein_adapter_memory(void)
{
  l4_addr_t address;
  l4_msgdope_t result;
  l4_snd_fpage_t sfpage;
  int error;

  l4_addr_t adapter_space_start = mem_lower ? (mem_lower << 10) : 0x9F000;
  l4_addr_t adapter_space_end   = 1 << 20;

  for (address = adapter_space_start;
       address < adapter_space_end;
       address +=  L4_PAGESIZE)
    {
      error = l4_ipc_call(my_pager, L4_IPC_SHORT_MSG,
			  SIGMA0_REQ_FPAGE_IOMEM_CACHED, 
			  l4_fpage(address, L4_LOG2_PAGESIZE, 0, 0).fpage, 
			  L4_IPC_MAPMSG(0, L4_WHOLE_ADDRESS_SPACE),
			  &sfpage.snd_base, &sfpage.fpage.fpage,
			  L4_IPC_NEVER, &result);

      if (error)
        boot_panic("can't map adapter space (IPC error 0x%02x)",
	    error);
    }

  region_add(adapter_space_start, adapter_space_end, -1, "Adapter Space Area");
}
#endif

/**
 * page in memory we own and we reserved before booting; we need to
 * do this explicitly as the L4 kernel won't hand it out voluntarily
 * (see L4 rerefence manual, sec. 2.7).
 * don't care for overlaps with our own area here because we will
 * reserve our memory below
 */
static void
reserve_module_memory(void)
{
  l4_addr_t a;

  if (!mb_info->mods_count)
    return;

  unsigned i = 3;
  mod_range_start = ~0UL;
  mod_range_end = 0;

  for (; i < mb_info->mods_count; ++i)
    {
      if (mb_mod[i].mod_start < mod_range_start)
	mod_range_start = mb_mod[i].mod_start;

      if (mb_mod[i].mod_end > mod_range_end)
	mod_range_end = mb_mod[i].mod_end;
    }

  /* Leave if no modules */
  if (!mod_range_end)
    return;

  for (a = l4_trunc_page(mod_range_start); a < mod_range_end; a += L4_PAGESIZE)
    {
      /* check if we really can touch the memory */
      if (a >= mem_high + ram_base)
	{
	  boot_panic("can't reserve memory at "l4_addr_fmt
	      	     " for boot modules. ", a);

	  printf("  The RAM at this address is owned by the kernel.\n");
	  if (l4_version == VERSION_FIASCO)
	    printf(" Fiasco occupies 20%% of the\n"
		   "      physical memory per default.\n");
	  break;
	}

      memmap_set_page(a, O_RESERVED);
      reserved_size += L4_PAGESIZE;

      /* force write page fault */
      l4_touch_rw((void *)a, 4);
    }

  region_add(mod_range_start, mod_range_end, -1, "Boot Modules");
}

/**
 * Reserve memory for ourselves; page in our own memory en-passant.
 */
static void
reserve_rmgr_memory(void)
{
  l4_addr_t start = (l4_addr_t)&_start;
  l4_addr_t end   = (l4_addr_t)&_end;
  l4_addr_t a;

  for (a = l4_trunc_page(start); a <= end; a += L4_PAGESIZE)
    {
      memmap_set_page(a, O_RESERVED);
      reserved_size += L4_PAGESIZE;

      /* force write page fault */
      l4_touch_rw((void *)a, 4);
    }

  region_add(start, end, -1, "Roottask");
}


/**
 * Reserve memory for Fiasco symbols.
 * Move it to the upper memory limit to prevent memory fragmentation
 */
static void
reserve_symbols_memory(void)
{
#if defined ARCH_x86 | ARCH_amd64
  l4_addr_t start = mb_mod[symbols_module].mod_start;
  l4_addr_t end   = mb_mod[symbols_module].mod_end;
  l4_addr_t size  = end - start;
  l4_addr_t pages = (l4_round_page(end) - l4_trunc_page(start)) / L4_PAGESIZE;
  l4_addr_t i, a, address;
  l4_threadid_t tid;

  if (0 == (address = find_free_chunk(pages, 1)))
    {
       boot_warning("found no memory for fiasco symbols, "
		    "disabling Fiasco symbols\n");
       return;
    }

  memcpy((void*)address, (void*)start, size);

  for (i=0, a=address; i<pages; i++, a += L4_PAGESIZE)
    {
      memmap_set_page(a, O_DEBUG);
      reserved_size += L4_PAGESIZE;
    }

  tid.id.task = 0; /* kernel space */
  fiasco_register_symbols(tid, address, size);

  region_add(address, address+size, -1, "Fiasco Symbols");
#else
  printf("Loading Fiasco symbols not supported.\n");
#endif

  free_module_image(symbols_module);
}

/**
 * Reserve memory for Fiasco lines.
 * Move it to the upper memory limit to prevent memory fragmentation
 */
static void
reserve_lines_memory(void)
{
#if defined ARCH_x86 | ARCH_amd64
  l4_addr_t start = mb_mod[lines_module].mod_start;
  l4_addr_t end   = mb_mod[lines_module].mod_end;
  l4_addr_t size  = end - start;
  l4_addr_t pages = (l4_round_page(end) - l4_trunc_page(start)) / L4_PAGESIZE;
  l4_addr_t i, a, address;
  l4_threadid_t tid;

  if (0 == (address = find_free_chunk(pages, 1)))
    {
      boot_warning("found no memory for fiasco lines, "
	           "disabling Fiasco Lines\n");
      return;
    }

  memcpy((void*)address, (void*)start, size);

  for (i=0, a=address; i<pages; i++, a += L4_PAGESIZE)
    {
       memmap_set_page(a, O_DEBUG);
       reserved_size += L4_PAGESIZE;
    }

  tid.id.task = 0; /* kernel space */
  fiasco_register_lines(tid, address, size);

  region_add(address, address+size, -1, "Fiasco Lines");
#else
  printf("Loading of Fiasco Lines not supported.\n");
#endif

  free_module_image(lines_module);
}


static void
check_for_ux(void)
{
  const char *version_str;

  kip = map_kip(0x1000);

  version_str = (const char*)
		    ((l4_addr_t)kip + (kip->offset_version_strings << 4));

  if (strstr(version_str, "(ux)"))
    {
      puts("  Found Fiasco-UX.");
      ux_running = 1;
    }

  l4_fpage_unmap(l4_fpage((l4_umword_t) kip, L4_LOG2_PAGESIZE, 0, 0),
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
  kip = 0;
}

/**
 * Reserve address 0x1000 for the kernel info page.
 *
 * we assume the L4 kernel is loaded to 0x1000, creating a virtual
 * memory area which doesn't contain mapped RAM so we can use it as
 * a scratch area.  this is at least consistent with the L4
 * reference manual, appendix A.  however, we flush the page anyway
 * before attempting to map something in there, just in case.
 */
static void
reserve_KIP(void)
{
  kip = map_kip(0x1000+ram_base);

  memmap_set_page((l4_umword_t) kip, O_RESERVED);
  reserved_size += L4_PAGESIZE;

  region_add((l4_addr_t)kip, (l4_addr_t)kip + L4_PAGESIZE, -1, "KIP");
}

/**
 * This function reserves memory occupied by loaded modules in the
 * global memory map.
 */
static void
reserve_task_memory (l4_addr_t start, l4_addr_t end, int task_no, int mod_no)
{
  l4_addr_t address;
  int region_no;

  printf("["l4_addr_fmt"-"l4_addr_fmt"]", start, end);

  for (address = l4_trunc_page(start); address < end; address += L4_PAGESIZE)
    {
      // XXX this check is also done by memmap_alloc_page
      if (!quota_check_mem(task_no, address, L4_PAGESIZE))
        boot_panic("can't allocate page at 0x"l4_addr_fmt": not in quota", 
	           address);

      if (!memmap_alloc_page(address, task_no))
	{
	  printf("\nRoottask: cannot load binary "
		 "because address at "l4_addr_fmt" not free\n", address);

	  if ((region_no = (region_overlaps(start, end))))
	    {
	      printf("  loaded module:   [%08x-%08x) ",
		  mb_mod[mod_no].mod_start, mb_mod[mod_no].mod_end);
	      print_module_name(get_module_name(&mb_mod[mod_no], 
			                        "[UNKNOWN]"), 38, 38);
	      printf("\n  overlaps with: ");
	      region_print(region_no);
	    }
	  else
	      printf("  for unkown reason\n");

	  boot_panic("can't allocate page at 0x"l4_addr_fmt": owned by #%02x",
	      address, memmap_owner_page(address));
	}
    }

  region_add(start, end, task_no,
	     (char*)get_module_name(&mb_mod[mod_no], "task"));
}

/**
 * the non-RAM high-memory superpages are all free for allocation.
 */
static void
free_high_ram(void)
{
  l4_addr_t page;
  for (page = l4_round_superpage(mem_high+ram_base) >> L4_SUPERPAGESHIFT;
      page < SUPERPAGE_MAX; page++)
    memmap_free_superpage(page << L4_SUPERPAGESHIFT, O_RESERVED);
}

/**
 * free unused __memmap and complete __pool
 */
static void
free_unused_memory(void)
{
  l4_addr_t address;
  const l4_addr_t free_beg = l4_round_page(__memmap + mem_high/L4_PAGESIZE);
  const l4_addr_t free_end = l4_round_page(&_end);

  for (address  = free_beg; address < free_end; address += L4_PAGESIZE)
    {
      memset((void*)address, 0, L4_PAGESIZE);
      if (!memmap_free_page(address, O_RESERVED))
	printf ("ROOT: Error freeing __memmap at "l4_addr_fmt"\n", address);
    }

  region_free(free_beg, free_end);
}

/* get the total amount of RAM available in the system, as reported
 * by the memory descriptors */
static unsigned long
get_total_ram(void)
{
  l4_kernel_info_mem_desc_t *md = l4_kernel_info_get_mem_descs(kip);
  unsigned num = l4_kernel_info_get_num_mem_descs(kip);
  unsigned long total_ram = 0;

  for (; num--; md++)
    if (!l4_kernel_info_get_mem_desc_is_virtual(md)
        && (l4_kernel_info_get_mem_desc_type(md) == l4_mem_type_conventional))
      total_ram += l4_kernel_info_get_mem_desc_end(md)
                   - l4_kernel_info_get_mem_desc_start(md) + 1;

  return total_ram;
}

/**
 * initialize the memory mangement.
 */
static void
init_memmap(void)
{
  unsigned long total_ram;

  assert(mem_high > 0x200000);	/* 2MB is the minimum for any useful work */

  /* initialize memory to "reserved" */
  memmap_init();
  pager_init();
  region_init();

  if (!no_pentium)
    pagein_4MB_memory();
#if defined ARCH_x86 | ARCH_amd64
  /* page in special memory regions XXX the order is important */
  pagein_bios_memory();
  pagein_adapter_memory();
#endif
  pagein_4KB_memory();

  /* reserve special memory regions */
  reserve_module_memory();
  reserve_rmgr_memory();
  if (fiasco_symbols)
    reserve_symbols_memory();
  if (fiasco_lines)
    reserve_lines_memory();
  reserve_KIP();

  free_high_ram();

  total_ram = get_total_ram();

  printf("\n"
	 " %7ldkB (%4ldMB) total RAM (reported by bootloader)\n"
	 " %7ldkB (%4ldMB) received RAM from Sigma0\n"
	 " %7ldkB (%4ldMB) reserved RAM for RMGR\n",
	 total_ram >> 10, total_ram >> 20,
	 (unsigned long)(free_size    +(1<<10)-1) / (1<<10),
	 (unsigned long)(free_size    +(1<<20)-1) / (1<<20),
	 (unsigned long)(reserved_size+(1<<10)-1) / (1<<10),
	 (unsigned long)(reserved_size+(1<<20)-1) / (1<<20));

  if (!verbose)
    return;

  putchar('\n');
  memmap_dump();
  putchar('\n');
  regions_dump();
}



/**
 * ask the pager for ioports to receive.
 */
static void
init_iomap(void)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  l4_msgdope_t result;
  l4_fpage_t fp;
  int error;
  unsigned p;
  l4_umword_t ignore;

  /* initialize the IO space to "reserved" */
  iomap_init();

  /* only continue and try to get ports if we are running on FIASCO */
  if(l4_version != VERSION_FIASCO)
    return;

  /* try get the whole IO space at once */
  error = l4_ipc_call(my_pager, L4_IPC_SHORT_MSG,
		      l4_iofpage(0, L4_WHOLE_IOADDRESS_SPACE, 0).fpage, 0,
		      L4_IPC_IOMAPMSG(0, L4_WHOLE_IOADDRESS_SPACE),
		      &ignore, &fp.fpage,
		      L4_IPC_NEVER, &result);
  /* XXX should i check for errors? */

  if (l4_ipc_fpage_received(result) /* got something */
      && l4_is_io_page_fault(fp.raw)/* got IO ports */
      && fp.iofp.iosize == L4_WHOLE_IOADDRESS_SPACE
      && fp.iofp.iopage == 0)       /* got whole IO space */
    {
      for(p = 0; p < RMGR_IO_MAX; p++)
	iomap_free_port(p, O_RESERVED);

      printf("  Received I/O ports 0000-ffff\n");
      have_io = 1;
    }
  else
    printf("  Received no I/O ports\n");
#endif
}

/**
 * find a free page and allocate the page.
 * XXX perhaps move to memmap.c later XXX.
 */
static l4_addr_t
find_free_page(unsigned task_no)
{
  l4_addr_t a;

  /* skip page 0, 1, and 2 */
  for (a = 0x3000 + ram_base; a - ram_base < mem_high; a += L4_PAGESIZE)
    {
      if (! quota_check_mem(task_no, a, L4_PAGESIZE))
	continue;
      if (memmap_owner_page(a) != O_FREE)
	continue;
      if (memmap_alloc_page(a, task_no))
	break;
    }

  if (a - ram_base >= mem_high)
    return 0;

  memset((void*)a, 0, L4_PAGESIZE);
  return a;
}

/**
 * check if the object is lying in one page alloc this part of the new page.
 */
static void*
alloc_from_page(l4_addr_t *addr, l4_size_t size)
{
  l4_addr_t old_addr = *addr;
  l4_addr_t new_addr = (*addr + size + 3) & ~3;

  if (l4_trunc_page(*addr) != l4_trunc_page(new_addr))
    return NULL;

  *addr = new_addr;
  return (void*)old_addr;
}

/**
 * reserve continued page range.
 * XXX perhaps move to memmap.c later XXX.
 */
static int
memmap_alloc_range(l4_addr_t start, l4_addr_t end, unsigned task_no)
{
  l4_addr_t address;

  /* change owner of module memory image */
  for (address = l4_trunc_page(start);
		 address < end;
		 address += L4_PAGESIZE)
  {
    if (!quota_check_mem(task_no, address, L4_PAGESIZE))
      {
	boot_error("can't allocate page at 0x"l4_addr_fmt" "
	       "for task %x (not in quota)\n"
	       "       skipping rest of module\n",
		address, task_no);
	return 0;
      }

    if (!memmap_alloc_page(address, task_no))
      {
	boot_error("can't allocate page at 0x"l4_addr_fmt" "
	       "for this task (owned by 0x%x)\n"
	       "       skipping rest of module",
		address, memmap_owner_page(address));
	memmap_dump();
	enter_kdebug("b");
	return 0;
      }
  }

  return 1;
}

/**
 * if any modules have been passed on to this task, copy module info.
 **/
static int
add_boot_modules(l4util_mb_mod_t *m, l4_addr_t *address,
		 unsigned mod_no, unsigned task_no)
{
  unsigned copy_mod_no = 0; // new number of passed module
  unsigned copy_mods_cnt = bootquota_get(task_no)->mods;

  /* copy mb_mod structures for the modules to be passed */
  for (; copy_mods_cnt > 0; mod_no++, copy_mod_no++, copy_mods_cnt--)
    {
      if (mod_no >= mb_info->mods_count)
        {
          boot_warning("task configured to use modules, "
		       "but there are not enough modules"
		       " for loading --> not passing\n");
          return copy_mod_no;
	}

      printf("     passing module ");
      print_module_name(get_module_name(&mb_mod[mod_no], "[UNKNOWN]"), 37, 37);
      printf(" [ %08x-%08x ]\n",
	  mb_mod[mod_no].mod_start, mb_mod[mod_no].mod_end);
      m[copy_mod_no] = mb_mod[mod_no];

      /* copy module command line */
      if (m[copy_mod_no].cmdline)
	{
	  l4_size_t len = strlen(L4_CHAR_PTR (m[copy_mod_no].cmdline)) + 1;
          char *string = L4_CHAR_PTR alloc_from_page(address, len);

	    if (!string)
	      {
		 boot_warning("can't pass command line --> disabling \"%s\"\n",
			     L4_CHAR_PTR (m[copy_mod_no].cmdline));

		 m[copy_mod_no].cmdline = 0;
		 continue;
	       }

	   strcpy(string, L4_CHAR_PTR (m[copy_mod_no].cmdline));
	   m[copy_mod_no].cmdline = (l4_addr_t) string;
	}

      // give reserved module memmory free before allocate it by task
      memmap_set_range(m[copy_mod_no].mod_start,
		       m[copy_mod_no].mod_end, O_FREE);
      memmap_alloc_range(m[copy_mod_no].mod_start,
			 m[copy_mod_no].mod_end, task_no);
    }

  return copy_mod_no;
}

/**
 * copy multi boot info structure for loaded tasks.
 */
static l4util_mb_info_t*
copy_mbi(l4_addr_t *tramp_page,
	 unsigned task_no, const char *name, unsigned *mod_no)
{
  l4_addr_t save_address;
  l4util_mb_info_t *mbi;
  l4util_mb_vbe_mode_t *mbi_vbe_mode;
  l4util_mb_vbe_ctrl_t *mbi_vbe_ctrl;
  l4util_mb_mod_t *m;
  unsigned mods_cnt;

  /* copy mb_info */
  mbi = (l4util_mb_info_t*) alloc_from_page(tramp_page,
					    sizeof(l4util_mb_info_t));
  if (!mbi)
    boot_panic("can't pass MBI info");

  *mbi = *mb_info;

  /* copy mb_info->mb_vbe_mode and mb_info->mb_vbe_ctrl */
  if (mb_info->flags & L4UTIL_MB_VIDEO_INFO)
    {
      save_address = *tramp_page;
      mbi_vbe_mode = 
	(l4util_mb_vbe_mode_t*)alloc_from_page(tramp_page,
					       sizeof(l4util_mb_vbe_mode_t));
      mbi_vbe_ctrl =
	(l4util_mb_vbe_ctrl_t*)alloc_from_page(tramp_page,
					       sizeof(l4util_mb_vbe_ctrl_t));

      if (!mbi_vbe_mode || !mbi_vbe_ctrl)
	{
	   boot_warning("can't pass VBE video info --> disabling\n ");
	   mbi->flags &= ~L4UTIL_MB_VIDEO_INFO;
	   *tramp_page = save_address;
	}
      else
	{
          *mbi_vbe_mode = *mb_vbe_mode;
	  mbi->vbe_mode_info = (l4_addr_t)mbi_vbe_mode;

	  *mbi_vbe_ctrl = *mb_vbe_ctrl;
	  mbi->vbe_ctrl_info = (l4_addr_t)mbi_vbe_ctrl;
	}
    }

  // copy mb_info->cmdline
  if (mb_mod[*mod_no].cmdline)
    {
	char *string;
	l4_size_t len = strlen(name) + 1;

	if (!(string = (char*) alloc_from_page(tramp_page, len)))
	  {
	    boot_warning("can't pass command line --> disabling\n");
 	    mbi->flags &= ~L4UTIL_MB_CMDLINE;
	  }
	else
	  {
	    mbi->flags |= L4UTIL_MB_CMDLINE;
	    mbi->cmdline = (l4_addr_t) string;
	    snprintf(string, len, "%s", name);
	  }
    }

  // copy boot modules to be passed
  mbi->mods_count = 0;
  mbi->mods_addr = 0;
  mbi->flags &= ~L4UTIL_MB_MODS;

  mods_cnt = bootquota_get(task_no)->mods;

  if (mods_cnt)
    {
      m =
	(l4util_mb_mod_t*)alloc_from_page(tramp_page, 
					  mods_cnt * sizeof(l4util_mb_mod_t));
      if (!m)
	{
	  boot_warning("can't pass module info --> disabling %s", name);
	  return 0;
	}

      mbi->mods_addr = (l4_addr_t) m;
      mbi->mods_count = add_boot_modules(m, tramp_page, (*mod_no)+1, task_no);
      mbi->flags |= L4UTIL_MB_MODS;

      *mod_no += mbi->mods_count;
    }

  return mbi;
}

/**
 * setup stack and trampoline page before starting task.
 */
static void
setup_stack_page(l4_addr_t *tramp_page,
		 l4_taskid_t t, l4_addr_t *entry, l4_umword_t **sp)
{
  l4_size_t s = (l4_addr_t)task_trampoline_end - (l4_addr_t)task_trampoline;
  void     *a = alloc_from_page(tramp_page, s);

  if (!a)
    boot_panic("can't allocate trampoline code");

  /* copy task_trampoline PIC code to new task's stack */
  memcpy(a, task_trampoline, s);
  *entry = (l4_addr_t)a;

  if (l4_round_page(*tramp_page) - *tramp_page < 0x80)
    boot_panic("can't allocate stack");

  *sp = (l4_umword_t*)l4_round_page(*tramp_page);
}

/**
 * extract symbols and lines as debug information.
 */
static void
setup_symbols_and_lines(l4_addr_t mod_start, l4_threadid_t t)
{
  l4_umword_t from_sym = 0, to_sym = 0;
  l4_umword_t from_lin = 0, to_lin = 0;

  if (!no_symbols)
    extract_symbols_from_image(mod_start, t.id.task, &from_sym, &to_sym);

  if (!no_lines)
    extract_lines_from_image(mod_start, t.id.task, &from_lin, &to_lin);

  if (from_sym || from_lin)
    printf("     ");
  if (from_sym)
    printf("symbols at ["l4_addr_fmt"-"l4_addr_fmt"] (%ldkB)",
	from_sym, to_sym, (to_sym-from_sym)/1024);
  if (from_lin)
    printf("%slines at ["l4_addr_fmt"-"l4_addr_fmt"] (%ldkB)",
	from_sym ? ", " : "",
	from_lin, to_lin, (to_lin-from_lin)/1024);
  if (from_sym || from_lin)
    putchar('\n');
}

/**
 * support for loading tasks from boot modules and allocating memory for them.
 **/
static int
alloc_exec_read_exec(void *handle, l4_addr_t file_ofs, l4_size_t file_size,
		     l4_addr_t mem_addr, l4_size_t mem_size,
		     exec_sectype_t section_type)
{
  exec_task_t *e = (exec_task_t *) handle;
  int offset = quota_get_offset(e->task_no);

  if (! (section_type & EXEC_SECTYPE_ALLOC))
    return 0;
  if (! (section_type & (EXEC_SECTYPE_ALLOC|EXEC_SECTYPE_LOAD)))
    return 0;

  // task wants no 1:1 mapping
  if (offset)
    vm_add(e->task_no, mem_addr, mem_addr + mem_size, offset);

  reserve_task_memory (mem_addr + offset, mem_addr + mem_size + offset,
                       e->task_no, e->mod_no);

  memcpy((void *) mem_addr + offset, (char*)(e->mod_start) + file_ofs,
	file_size);
  if (file_size < mem_size)
    memset((void *) (mem_addr + offset + file_size), 0, mem_size - file_size);

  return 0;
}

static int
alloc_exec_read(void *handle, l4_addr_t file_ofs, void *buf, l4_size_t size,
		l4_size_t *out_actual)
{
  exec_task_t *e = (exec_task_t *) handle;
  memcpy(buf, (char*)(e->mod_start) + file_ofs, size);

  *out_actual = size;
  return 0;
}

/**
 * start tasks the rest of booted task except the one that should be startet
 * by other tasks (see copy_mbi).
 */
static void
start_tasks(void)
{
  l4_threadid_t t;
  l4_umword_t *sp = 0;
  l4util_mb_info_t *mbi;
  const char *name, *error_msg;
  l4_addr_t task_entry, tramp_entry, tramp_page;
  int exec_ret;
  exec_task_t e;
  unsigned task_no; // number of currently bootet task
  unsigned mod_no;  // number of currently handled boot module

  printf("\nRoottask: Loading %d module%s.\n",
         mb_info->mods_count - first_task_module,
	 (mb_info->mods_count - first_task_module) == 1 ? "" : "s");

  t          = myself;

  for (mod_no = first_task_module, task_no = myself.id.task+1;
       mod_no < mb_info->mods_count;
       mod_no++, task_no++)
    {
      t.id.task = task_no;
      if (!task_alloc(task_no, myself.id.task))
	boot_panic("can't allocate task");

      name = get_module_name(&mb_mod[mod_no], "");

      // set some resources configured for this task
      names_set(t, name);
      cfg_quota_copy(task_no, name);
      cfg_bootquota_copy(task_no, name);

      if (l4_version == VERSION_FIASCO)
	printf("\033[36m");
      printf("#%02x: loading \"%s\"\n", task_no, name);
      if (l4_version == VERSION_FIASCO)
	printf("\033[m");
      printf("     from [\033[37m%08x\033[m-\033[37m%08x\033[m] to ",
	     mb_mod[mod_no].mod_start, mb_mod[mod_no].mod_end);

      e.mod_start = L4_VOID_PTR (mb_mod[mod_no].mod_start);
      e.task_no   = task_no;
      e.mod_no    = mod_no;
      exec_ret    = exec_load_elf(alloc_exec_read, alloc_exec_read_exec,
				  &e, &error_msg, &task_entry);
      putchar('\n');

      if (exec_ret)
	{
	  boot_error("can't load module (%s)", error_msg);
	  continue;
	}

      /* pass multiboot info and stack of new task in extra page */
      if (!(tramp_page = find_free_page(task_no)))
	boot_error("can't allocate page");

      else
	{
	  /* copy mbi for task and all needed modules */
	  mbi = copy_mbi(&tramp_page, task_no, name,  &mod_no);

    	  /* setup stack for task_trampoline() */
	  setup_stack_page(&tramp_page, t, &tramp_entry, &sp);
	  *--sp = (l4_umword_t) mbi;
	  *--sp = task_entry;
	  *--sp = 0;		/* set SP to final position */

	  printf("     entry at \033[37m"l4_addr_fmt
		 "\033[m via trampoline page code\n", tramp_entry);

	  setup_symbols_and_lines((l4_addr_t)e.mod_start, t);

	  /* free the memory that module image occupied
	   * (don't use mod_no here, it's overwritten by copy_mbi!) */
	  free_module_image(e.mod_no);

	  t = l4_task_new(t, bootquota_get(task_no)->mcp,
			  (l4_umword_t) sp, tramp_entry, myself);

	  setup_task(t);
	}
    }

  putchar('\n');
}

/**
 * Started as the L4 booter task through startup() from Bootstrap.
 */
void
init(void)
{
  puts("\n\nRoottask.");

  init_rmgr();
  check_for_ux();

  reset_malloc();
  init_config();
  init_memmap();
  init_iomap();
  init_irq();
  init_task();
  init_quota();
  init_small();

  configure();

  setup_symbols_and_lines((l4_addr_t)mb_mod[2].mod_start, myself);
  setup_task(myself);
  setup_task(my_pager);

  /* start the tasks loaded as modules */
  start_tasks();

  /* free unused __memmap and complete __pool */
  free_unused_memory();

  /* now start the resource manager */
  rmgr_main(memdump);
}
