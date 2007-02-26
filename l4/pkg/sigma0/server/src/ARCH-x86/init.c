/* startup stuff */

/* it should be possible to throw away the text/data/bss of the object
   file resulting from this source -- so, we don't define here
   anything we could still use at a later time.  instead, globals are
   defined in globals.c */

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/ktrace.h>
#include <l4/util/mb_info.h>
#ifndef USE_OSKIT
#include <l4/env_support/panic.h>
#endif

#include "config.h"
#include "memmap.h"
#include "globals.h"
#include "init.h"

static void      init_globals(void);
static void      init_memmap(void);
static l4_addr_t startup_alloc(l4_size_t size);
static void      add_reserved(l4_addr_t begin, l4_addr_t end, int alloc);

static struct
{
  l4_addr_t low, high;
  char alloc;
} memmap_reserved[24];

/* started as the L4 sigma0 task from crt0.S */
void 
init(l4util_mb_info_t *mbi, unsigned int flag, l4_kernel_info_t *info)
{
  l4_addr_t address;

#ifdef FIASCO_UX
  if ((mbi->flags & L4UTIL_MB_MODS) && mbi->mods_addr)
    {
      l4util_mb_mod_t *m = (l4util_mb_mod_t*)mbi->mods_addr;
      int i;

      /* recognize the "-quiet" switch */
      if (strstr((char *)m->cmdline, "-quiet"))
	quiet = 1;

      /* make sure that __memmap and __memmap4mb aren't located at
       * memory where boot modules are loaded */
      for (i=0; i<mbi->mods_count; i++)
	add_reserved(m[i].mod_start, m[i].mod_end, 0);
    }
#endif

  printf("SIGMA0: Hello!\n");

  l4_info = info;

  if (l4_info->version == 0x01004444)
    {
      char kip_syscalls = l4_info->reserved[3];
      tbuf_status = fiasco_tbuf_get_status_phys();

      printf("  Found Fiasco: KIP syscalls: %s.\n",
	  kip_syscalls ? "yes" : "no");
    }

  init_globals();
  init_memmap();

  /* add the memory used by this module and not any longer required
     (the ".init" section) to the free memory pool */
  for (address = (l4_addr_t) &__crt_dummy__ & L4_PAGEMASK;
       address < ((l4_addr_t) &_stext & L4_PAGEMASK);
       address += L4_PAGESIZE)
    check(memmap_free_page(address, O_RESERVED));

  /* now start the memory manager */
  pager();
}


/* support functions for init() */

static
void
init_globals(void)
{
  l4_umword_t dummy;

  /* set myself (my thread id) */
  myself = l4_myself();

  /* set my_pager */
  my_preempter = my_pager = L4_INVALID_ID;
  l4_thread_ex_regs(myself, (l4_umword_t) -1, (l4_umword_t) -1, 
		    &my_preempter, &my_pager, &dummy, &dummy, &dummy);

  /* bug compatibility with Jochen's L4 */
  jochen = ! (l4_info->version & 0xffff0000); /* jochen's versions have
						 high-word unset */
}

static
void
init_memmap(void)
{
  l4_addr_t address;
  l4_size_t size, size_sum = 0;
  int i;

  /* find the kernel info page */
  assert(l4_info->magic == L4_KERNEL_INFO_MAGIC);
  assert(l4_info->sigma0_memory.low <= (l4_addr_t)&_stext);
  assert(l4_info->sigma0_memory.high >= (l4_addr_t)&_end);

  mem_high = l4_trunc_page(l4_info->main_memory.high);

  /* reserve areas forbidden for __memmap, __memmap4mb, and __iomap */
  add_reserved(l4_info->reserved0.low,     l4_info->reserved0.high,     1);
  add_reserved(l4_info->reserved1.low,     l4_info->reserved1.high,     1);
  add_reserved(l4_info->semi_reserved.low, l4_info->semi_reserved.high, 1);
  add_reserved(l4_info->sigma0_memory.low, l4_info->sigma0_memory.high, 1);
  add_reserved(l4_info->sigma1_memory.low, l4_info->sigma1_memory.high, 1);
  add_reserved(l4_info->root_memory.low,   l4_info->root_memory.high,   2);
  for (i=0; i<4; i++)
    add_reserved(l4_info->dedicated[i].low, l4_info->dedicated[i].high, 2);

  /* allocate __memmap */
  size = (mem_high/L4_PAGESIZE)*sizeof(__memmap[0]);
  if (!(__memmap = (owner_t*)startup_alloc(size)))
    panic("Cannot allocate memory for __memmap");
  add_reserved((l4_addr_t)__memmap, (l4_addr_t)__memmap+size-1, 1);
  size_sum += size;

  /* allocate __memmap4mb */
  size = SUPERPAGE_MAX*sizeof(__memmap4mb[0]);
  if (!(__memmap4mb = (__superpage_t*)startup_alloc(size)))
    panic("Cannot allocate memory for __memmap4mb");
  add_reserved((l4_addr_t)__memmap4mb, (l4_addr_t)__memmap4mb+size-1, 1);
  size_sum += size;

#ifndef FIASCO_UX
  /* allocate __iomap */
  size = IO_MAX*sizeof(__iomap[0]);
  if (!(__iomap = (owner_t*)startup_alloc(size)))
    panic("Cannot allocate memory for __iomap");
  add_reserved((l4_addr_t)__iomap, (l4_addr_t)__iomap+size-1, 1);
  size_sum += size;
#endif

  printf("  Allocated %dkB for maintenance structures.\n",
	 (size_sum+1023)/1024);

  /* initialize memory pages to "reserved" and io ports to free */
  memmap_init();

  /* free all non-reserved memory: first, free all, then reserve stuff */
  for (address = l4_trunc_page(l4_info->main_memory.low);
       address < l4_round_page(l4_info->main_memory.high);
       address += L4_PAGESIZE)
    memmap_free_page(address, O_RESERVED);

  for (i=0; i<sizeof(memmap_reserved)/sizeof(memmap_reserved[0]); i++)
    if (memmap_reserved[i].low && memmap_reserved[i].alloc)
      {
	l4_addr_t begin = memmap_reserved[i].low;
	l4_addr_t end   = memmap_reserved[i].high;
	owner_t   owner = memmap_reserved[i].alloc == 1 ? O_RESERVED 
							: ROOT_TASKNO;

	for (address=begin; address<end; address+=L4_PAGESIZE)
	  memmap_alloc_page(address, owner);
      }

  /* the non-RAM high-memory superpages are all free for allocation */
  for (address = 0x8000; address < 0x10000;
       address += L4_SUPERPAGESIZE/0x10000) /* scaled by 0x10000**-1 to
					       prevent overflow */
    memmap_free_superpage(address * 0x10000, O_RESERVED);
}

static
void
add_reserved(l4_addr_t begin, l4_addr_t end, int alloc)
{
  int i;

  if (!begin && !end)
    return;

  begin = l4_trunc_page(begin);
  end   = l4_round_page(end);

  for (i=0; i<sizeof(memmap_reserved)/sizeof(memmap_reserved[0]); i++)
    {
      if (memmap_reserved[i].alloc == alloc)
	{
	  /* try to merge reserved region */
	  if (end == memmap_reserved[i].low)
	    {
	      memmap_reserved[i].low = begin;
	      return;
	    }
	  if (begin == memmap_reserved[i].high)
	    {
	      memmap_reserved[i].high = end;
	      return;
	    }
	}
      if (memmap_reserved[i].low == 0)
	{
	  memmap_reserved[i].low   = begin;
	  memmap_reserved[i].high  = end;
	  memmap_reserved[i].alloc = alloc;
	  return;
	}
    }

  panic("memmap_reserved too small");
}

static
l4_addr_t
startup_alloc(l4_size_t size)
{
  l4_addr_t p = l4_info->main_memory.high;
  int i;

  for (;;)
    {
loop:
      p = l4_trunc_page(p-size);

      if (p < 0x00100000)
	return 0;

      for (i=0; i<sizeof(memmap_reserved)/sizeof(memmap_reserved[0]); i++)
	if (memmap_reserved[i].low)
	  {
	    l4_addr_t begin = memmap_reserved[i].low;
	    l4_addr_t end   = memmap_reserved[i].high;

	    /* reserved areas at begin ... end-1 */
	    if ((p>=begin && p<end) || (p<begin && p+size>begin))
	      {
		p = begin;
		goto loop;
	      }
	  }

      return p;
    }
}
