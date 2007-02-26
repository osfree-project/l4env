/* $Id$ */
/**
 * \file	bootstrap/server/src/startup.c
 * \brief	Main functions
 *
 * \date	09/2004
 * \author	Torsten Frenzel <frenzel@os.inf.tu-dresden.de>,
 *		Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *		Adam Lackorzynski <adam@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

/* LibC stuff */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

/* L4 stuff */
#include <l4/sys/compiler.h>
#include <l4/sys/kdebug.h>
#include <l4/util/mb_info.h>
#include <l4/util/l4_macros.h>
#include <l4/env_support/panic.h>

/* local stuff */
#include "exec.h"
#include "macros.h"
#include "region.h"
#include "module.h"
#include "startup.h"
#include "init_kip.h"
#include "patch.h"
#if defined (ARCH_x86) || defined(ARCH_amd64)
#include "ARCH-x86/serial.h"
#endif

#if defined(REALMODE_LOADING) || defined(XEN) || defined(ARCH_arm)
#include "loader_mbi.h"
#endif

#undef getchar



extern int _start;	/* begin of image -- defined in crt0.S */
extern int _end;	/* end   of image -- defined by bootstrap.ld */

/* temporary buffer for multiboot info */
static l4util_mb_info_t *mb_info;
static l4util_mb_mod_t *mb_mod;
static boot_info_t boot_info;

const l4_addr_t _mod_addr = MODADDR;

/* hardware config variables */
int hercules = 0; /* hercules monitor attached */

/* modules to load by bootstrap */
static const int kernel = 1; /* we have at least a kernel */
#ifdef XEN
static int sigma0 = 0;   /* we need sigma0 */
static int roottask = 0; /* we need a roottask */
#else
static int sigma0 = 1;   /* we need sigma0 */
static int roottask = 1; /* we need a roottask */
#endif

enum {
  kernel_module,
  sigma0_module,
  roottask_module,
};

/* RAM memory size */
static l4_addr_t mod_range_start = 0;
static l4_addr_t mod_range_end = 0;

static l4_addr_t bios_area_start = 0;
static l4_addr_t bios_area_end = 0;

static l4_addr_t bootstrap_start = 0;
static l4_addr_t bootstrap_end = 0;

/* we define a small stack for sigma0 and roottask here -- it is used by L4
 * for parameter passing. however, sigma0 and roottask must switch to its
 * own private stack as soon as it has initialized itself because this memory
 * region is later recycled in init.c */
static char roottask_init_stack[64]; /* XXX hardcoded */
static char sigma0_init_stack[64]; /* XXX hardcoded */

/* entry point */
void startup(l4util_mb_info_t *, l4_umword_t, void *);

static exec_handler_func_t l4_exec_read_exec;

static int
l4_exec_add_region(void * handle,
		  l4_addr_t file_ofs, l4_size_t file_size,
		  l4_addr_t mem_addr, l4_size_t mem_size,
		  exec_sectype_t section_type);
/**
 * copy mbi from bootstrap to roottask
 * also add the l4_version variable to command-line
 * don't copy kernel, sigma0 and roottask module
 */
static inline void*
lin_alloc(l4_size_t size, l4_uint8_t **ptr)
{
  void *ret = *ptr;
  *ptr += (size + 3) & ~3;;
  return ret;
}

#if 0
static void
dump_mbi(l4util_mb_info_t *mbi)
{
  printf("%p-%p\n", (void*)(mbi->mem_lower << 10), (void*)(mbi->mem_upper << 10));
  printf("MBI:     [%p-%p]\n", mbi, mbi + 1);
  printf("MODINFO: [%p-%p]\n", (char*)mbi->mods_addr,
      (l4util_mb_mod_t*)(mbi->mods_addr) + mbi->mods_count);

  printf("VBEINFO: [%p-%p]\n", (char*)mbi->vbe_ctrl_info,
      (l4util_mb_vbe_ctrl_t*)(mbi->vbe_ctrl_info) + 1);
  printf("VBEMODE: [%p-%p]\n", (char*)mbi->vbe_mode_info,
      (l4util_mb_vbe_mode_t*)(mbi->vbe_mode_info) + 1);

  l4util_mb_mod_t *m = (l4util_mb_mod_t*)(mbi->mods_addr);
  l4util_mb_mod_t *me = m + mbi->mods_count;
  for (; m < me; ++m)
    {
      printf("  MOD: [%p-%p]\n", (void*)m->mod_start, (void*)m->mod_end);
      printf("  CMD: [%p-%p]\n", (char*)m->cmdline,
	  (char*)m->cmdline + strlen((char*)m->cmdline));
    }
}
#endif

static
void *
find_kip(void)
{
  enum { L4_KERNEL_INFO_MAGIC = 0x4BE6344CL /* "L4µK" */ };
  unsigned char *p;
  void *k = 0;

  printf("  find kernel info page...\n");
  for (p = (unsigned char *) (boot_info.kernel_low & 0xfffff000);
       p < (unsigned char *) boot_info.kernel_low +
         (boot_info.kernel_high - boot_info.kernel_low);
       p += 0x1000)
    {
      l4_umword_t magic = L4_KERNEL_INFO_MAGIC;
      if (memcmp(p, &magic, 4) == 0)
	{
	  k = p;
	  printf("  found kernel info page at %p\n", p);
	  break;
	}
    }

  if (!k)
    panic("could not find kernel info page, maybe your kernel is too old");

  return k;
}

static inline
unsigned long get_api_version(void *kip)
{
  return ((unsigned long *)kip)[1];
}

/**
 * setup memory range
 */
static void
setup_memory_range(l4util_mb_info_t *mbi, boot_info_t *bi)
{
  char *c;

  if (mbi->flags & L4UTIL_MB_MEMORY)
    {
      bi->mem_upper = mbi->mem_upper;
      bi->mem_lower = mbi->mem_lower;
    }

  /* maxmem= parameter? */
  if ((mbi->flags & L4UTIL_MB_CMDLINE) &&
      (c = strstr(L4_CHAR_PTR(mbi->cmdline), " -maxmem=")))
    bi->mem_upper = 1024 * (strtoul(c + 9, NULL, 10) - 1);

  if (mbi->mem_upper != bi->mem_upper)
    printf("  Limiting memory to %ld MB (%ld KB)\n",
	bi->mem_upper/1024+1, bi->mem_upper+1024);

  bi->mem_high = (mbi->mem_upper + 1024) << 10;

  /* ensure that we have a valid multiboot info structure */
  mbi->mem_upper = bi->mem_upper;
  mbi->mem_lower = bi->mem_lower;
}

/**
 * init memory map
 */
static void
init_memmap(l4util_mb_info_t *mbi)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  bios_area_start = 0x09f000;
  bios_area_end   = 0x100000;
#endif

  bootstrap_start = (l4_addr_t)&_start;
  bootstrap_end   = (l4_addr_t)&_end;

  mod_range_start = (L4_MB_MOD_PTR(mbi->mods_addr))[0].mod_start;
  mod_range_end   = (L4_MB_MOD_PTR(mbi->mods_addr))[mbi->mods_count-1].mod_end;
}

/**
 * init region map
 */
static void
init_region(l4util_mb_info_t *mbi, boot_info_t *bi)
{
  region_add(bios_area_start, bios_area_end, ".BIOS area");
  region_add(bootstrap_start, bootstrap_end, ".bootstrap");
  region_add(mod_range_start, mod_range_end, ".Modules Memory");
  region_add(0,               RAM_BASE,      ".Deep Space");
#ifndef XEN
  region_add(RAM_BASE + bi->mem_high, ~0U,   ".Deep Space");
#endif
}

/**
 * load module
 */
static void
add_module_regions(l4util_mb_info_t *mbi, l4_umword_t module, l4_addr_t *begin, l4_addr_t *end)
{
  exec_task_t exec_task;
  l4_addr_t entry;
  int r;
  const char *error_msg;
  l4util_mb_mod_t *mb_mod = (l4util_mb_mod_t*)mbi->mods_addr;

  exec_task.begin = 0xffffffff;
  exec_task.end   = 0;

  exec_task.mod_start = L4_VOID_PTR(mb_mod[module].mod_start);
  exec_task.task_no   = module;
  exec_task.mod       = mb_mod + module;

  printf("  Scaning %s\n", L4_CHAR_PTR(mb_mod[module].cmdline));

  r = exec_load_elf(l4_exec_add_region, &exec_task,
                    &error_msg, &entry);
}

static l4_addr_t
load_module(l4_umword_t module, l4_addr_t *begin, l4_addr_t *end)
{
  exec_task_t exec_task;
  l4_addr_t entry;
  int r;
  const char *error_msg;

  exec_task.begin = 0xffffffff;
  exec_task.end   = 0;

  exec_task.mod_start = L4_VOID_PTR(mb_mod[module].mod_start);
  exec_task.task_no   = module;
  exec_task.mod       = mb_mod + module;

  r = exec_load_elf(l4_exec_read_exec, &exec_task,
                    &error_msg, &entry);

#ifndef RELEASE_MODE
  /* clear the image for debugging and security reasons */
  memset(L4_VOID_PTR(mb_mod[module].mod_start), 0,
         mb_mod[module].mod_end - mb_mod[module].mod_start);
#endif

  if (r)
    printf("  => can't load module (%s)\n", error_msg);

  *begin = exec_task.begin;
  *end   = exec_task.end;

  /* Only add region if non-elf module */
  if (r)
    region_add(exec_task.begin, exec_task.end,
	       L4_CHAR_PTR(mb_mod[module].cmdline));

  return entry;
}


static
unsigned char *
dup_cmdline(unsigned mod_nr, l4_uint8_t **ptr, char const *orig)
{
  char const *new_args = get_args_module(mod_nr);
  unsigned char *ret = *ptr;

  if (new_args)
    printf("new args for %d = %s\n", mod_nr, new_args);

  if (!orig)
    return 0;

  for (;*orig; ++(*ptr))
    {
      **ptr = *orig;
      if (new_args && (*orig == 0 || *orig == ' '))
	{
	  **ptr = ' ';
	  orig = new_args;
	  new_args = 0;
	}
      else
	++orig;
    }

  **ptr = 0;
  ++(*ptr);

  return ret;
}

/**
 * copy mbi to save place for boot
 */
static
l4util_mb_info_t *
relocate_mbi(l4util_mb_info_t *src_mbi, unsigned long* start,
             unsigned long* end)
{
  int i;
  l4util_mb_info_t *dst_mbi;
  l4_addr_t x = region_find_free(RAM_BASE + 0x2000, RAM_BASE + (4 << 20),
      16 << 10, L4_LOG2_PAGESIZE);
  if (!x)
    panic("no memory for mbi found (below 4MB)");

  void *mbi_start = (void*)x;

  l4_uint8_t *p = (l4_uint8_t*)x;
  *start = x;

  dst_mbi = lin_alloc(sizeof(l4util_mb_info_t), &p);

  printf("  Bootloader MMAP %s\n", src_mbi->flags & L4UTIL_MB_MEM_MAP ? "available" : "not available");

  l4util_mb_addr_range_t *mmap;
  for (mmap = (l4util_mb_addr_range_t *) src_mbi->mmap_addr;
       (unsigned long) mmap < src_mbi->mmap_addr + src_mbi->mmap_length;
       mmap = (l4util_mb_addr_range_t *) ((unsigned long) mmap + mmap->struct_size + sizeof (mmap->struct_size)))
    {
      char *types[] = { "unknown", "RAM", "reserved", "ACPI", "ACPI NVS", "unusable" };
      char *type_str = (mmap->type < sizeof(types)) ? types[mmap->type] : types[0];

      printf("    [%08x,%08x) %s (%d)\n",
             (unsigned) mmap->addr, (unsigned) mmap->addr + (unsigned) mmap->size,
             type_str, (unsigned) mmap->type);
    }

  /* copy (extended) multiboot info structure */
  memcpy(dst_mbi, src_mbi, sizeof(l4util_mb_info_t));

  dst_mbi->flags &= ~L4UTIL_MB_CMDLINE;

  if (dst_mbi->flags & L4UTIL_MB_MEM_MAP)
    {
      if (src_mbi->mmap_addr)
	{
	  void *mmap = lin_alloc(src_mbi->mmap_length, &p);
	  memcpy(mmap, (void*)src_mbi->mmap_addr, dst_mbi->mmap_length);
	  dst_mbi->mmap_addr = (unsigned long)mmap;
	}
      else
	dst_mbi->flags &= ~L4UTIL_MB_MEM_MAP;
    }


  /* copy extended VIDEO information, if available */
  if (dst_mbi->flags & L4UTIL_MB_VIDEO_INFO)
    {
      if (src_mbi->vbe_mode_info)
	{
	  l4util_mb_vbe_mode_t *m
	    = (l4util_mb_vbe_mode_t*)lin_alloc(sizeof(l4util_mb_vbe_mode_t),
		&p);

	  memcpy(m, L4_VOID_PTR(src_mbi->vbe_mode_info),
		 sizeof(l4util_mb_vbe_mode_t));
	  dst_mbi->vbe_mode_info = (l4_addr_t)m;
	}

      /* copy VBE controller info structure */
      if (src_mbi->vbe_ctrl_info)
	{
	  l4util_mb_vbe_ctrl_t *m
	    = (l4util_mb_vbe_ctrl_t*)lin_alloc(sizeof(l4util_mb_vbe_ctrl_t),
		&p);
	  memcpy(m, L4_VOID_PTR(src_mbi->vbe_ctrl_info),
		 sizeof(l4util_mb_vbe_ctrl_t));
	  dst_mbi->vbe_ctrl_info = (l4_addr_t)m;
	}
    }

  /* copy module descriptions */
  l4util_mb_mod_t *mods = (l4util_mb_mod_t*)lin_alloc(sizeof(l4util_mb_mod_t)*
      src_mbi->mods_count, &p);
  memcpy(mods, L4_VOID_PTR(dst_mbi->mods_addr),
	 dst_mbi->mods_count * sizeof (l4util_mb_mod_t));
  dst_mbi->mods_addr = (l4_addr_t)mods;
  mb_mod = mods;

  /* copy command lines of modules */
  for (i = 0; i < dst_mbi->mods_count; i++)
    {
      unsigned char *n = dup_cmdline(i, &p, (char const *)(mb_mod[i].cmdline));
      if (n)
	mods[i].cmdline = (l4_addr_t)n;
    }
  *end = (l4_addr_t)p;

  printf("  Relocated mbi to [%p-%p]\n", mbi_start, (void*)(*end));

  return dst_mbi;
}

#ifdef IMAGE_MODE

#ifdef COMPRESS
#include "uncompress.h"
#endif

typedef struct
{
  l4_uint32_t  start;
  l4_uint32_t  size;
  l4_uint32_t  size_uncompressed;
  l4_uint32_t  name;
} mod_info;

extern mod_info _module_info_start[];
extern mod_info _module_info_end[];

extern l4util_mb_mod_t _modules_mbi_start[];
extern l4util_mb_mod_t _modules_mbi_end[];

static void
construct_mbi(l4util_mb_info_t *mbi)
{
  int i;
  l4util_mb_mod_t *mods = _modules_mbi_start;

  mbi->mods_count  = _module_info_end - _module_info_start;
  mbi->flags      |= L4UTIL_MB_MODS;
  mbi->mods_addr   = (l4_addr_t)mods;

  assert(mbi->mods_count >= 2);

  for (i = 0; i < mbi->mods_count; i++)
    {
#ifdef COMPRESS
      l4_addr_t image =
	 (l4_addr_t)decompress(L4_CONST_CHAR_PTR(_module_info_start[i].name),
                               L4_VOID_PTR(_module_info_start[i].start),
                               _module_info_start[i].size,
                               _module_info_start[i].size_uncompressed);
#else
      l4_addr_t image = _module_info_start[i].start;
#endif
      mods[i].mod_start = image;
      mods[i].mod_end   = image + _module_info_start[i].size_uncompressed;
      printf("mod%02u: %08x-%08x: %s\n",
	     i, mods[i].mod_start, mods[i].mod_end,
	     L4_CHAR_PTR(_module_info_start[i].name));
    }
}
#endif /* IMAGE_MODE */

#ifdef REALMODE_LOADING
l4_addr_t _mod_end;

static void
move_modules_to_modaddr(void)
{
  int i, count = _module_info_end - _module_info_start;
  l4_uint8_t *a = (l4_uint8_t *)_mod_addr;

  /* patch module_info structure with new start values */
  for (i = 0; i < count; i++)
    {
      memcpy(a, L4_VOID_PTR(_module_info_start[i].start),
	    _module_info_start[i].size);
      _module_info_start[i].start = (l4_uint32_t)(l4_addr_t)a;
      a += l4_round_page(_module_info_start[i].size);
    }
  _mod_end = l4_round_page(a);
}
#else
extern char _module_data_start;
extern char _module_data_end;
l4_addr_t _mod_end   = (l4_addr_t)&_module_data_end;
#endif /* REALMODE_LOADING */

/**
 * \brief  Startup, started from crt0.S
 */
void
startup(l4util_mb_info_t *mbi, l4_umword_t flag,
	void *realmode_or_xen_si_pointer)
{
  void *l4i;
#if defined(ARCH_x86) || defined(ARCH_amd64)
  l4util_mb_info_t kernel_mbi;
#endif

#if defined(ARCH_x86) || defined(ARCH_amd64)
#ifdef REALMODE_LOADING
  mbi = init_loader_mbi(realmode_or_xen_si_pointer, 0);
  move_modules_to_modaddr();
#else
# ifdef XEN
  mbi = init_loader_mbi(NULL, MEMORY << 10);
# else
  assert(flag == L4UTIL_MB_VALID); /* we need to be multiboot-booted */
# endif
#endif
#else /* arm */
#ifdef ARCH_arm
  mbi = init_loader_mbi(NULL, MEMORY << 10);
#else
#error Unknown arch!
#endif
#endif

  /* parse command line */
  if (mbi->flags & L4UTIL_MB_CMDLINE)
    {
#if defined(ARCH_x86) || defined(ARCH_amd64)
      int comport = 1;
      const char *s;

      if ((s = strstr(L4_CHAR_PTR(mbi->cmdline), " -comport=")))
	comport = strtoul(s+10,0,0);

      if (strstr(L4_CHAR_PTR(mbi->cmdline), " -serial"))
	com_cons_init(comport);

      if (strstr(L4_CHAR_PTR(mbi->cmdline), " -hercules") && have_hercules())
        hercules = 1;
#endif

      if (strstr(L4_CHAR_PTR(mbi->cmdline), " -no-sigma0"))
	sigma0 = 0;

      if (strstr(L4_CHAR_PTR(mbi->cmdline), " -no-roottask"))
	roottask = 0;
    }

#ifdef IMAGE_MODE
  construct_mbi(mbi);
#endif

  /* We need at least two boot modules */
  assert(mbi->flags & L4UTIL_MB_MODS);
  assert(mbi->flags & L4UTIL_MB_MEMORY);
  /* We have at least the L4 kernel and the first user task */
  assert(mbi->mods_count >= 2);
  assert(mbi->mods_count <= MODS_MAX);

  /* we're not an L4 task yet -- we're just a GRUB-booted kernel! */
  puts("\nL4 Bootstrapper");

#ifdef ARCH_x86
  /* Limit memory, we cannot really handle more right now. In fact, the
   * problem is roottask. It maps as many superpages/pages as it gets.
   * After that, the remaining pages are mapped using l4sigma0_map_anypage()
   * with a receive window of L4_WHOLE_ADDRESS_SPACE. In response Sigma0
   * could deliver pages beyond the 3GB user space limit. */
  if (mbi->mem_upper << 10 > 3048UL << 20)
    mbi->mem_upper = 3048UL << 10;
#endif

  /* Guess the amount of installed memory from the bootloader. We already
   * need that value because (1) we have to initialize the memmap for module
   * loading and (2) we have to check for a valid L4 kernel. Later (in init.c),
   * mem_high is determined from the sigma0 server. */
  setup_memory_range(mbi, &boot_info);
  /* We initialize the memmap here to observe loading tasks before starting
   * the L4 kernel. Later in init.c, we re-initialize the memmap */
  init_memmap(mbi);

  /* We initialize the region map here to find overlapping regions and
   * report useful error messages. roottask will use its own region map */
  init_region(mbi, &boot_info);

  if (kernel)
    add_module_regions(mbi, kernel_module, &boot_info.kernel_low, 
	&boot_info.kernel_high);

  if (sigma0)
    add_module_regions(mbi, sigma0_module, &boot_info.sigma0_low,
	&boot_info.sigma0_high);

  if (roottask)
    add_module_regions(mbi, roottask_module,
	&boot_info.roottask_low, &boot_info.roottask_high);

  /* patch modules with content given at command line */
  if (mbi->flags & L4UTIL_MB_CMDLINE)
    {
      const char *s = L4_CONST_CHAR_PTR(mbi->cmdline);
      while ((s = strstr(s, " -patch=")))
	patch_module(&s, mbi);

      s = L4_CONST_CHAR_PTR(mbi->cmdline);
      while ((s = strstr(s, " -arg=")))
	args_module(&s, mbi);
    }

  /* copy Multiboot data structures, we still need to a safe place
   * before playing with memory we don't own and starting L4 */
  mb_info = relocate_mbi(mbi, &boot_info.mbi_low, &boot_info.mbi_high);
  if (!mb_info)
    panic("could not copy multiboot info to memory below 4MB");

  /* --- Shouldn't touch original Multiboot parameters after here. -- */

  /* setup kernel PART ONE */
  if (kernel)
    {
      printf("  Loading ");
      print_module_name(L4_CONST_CHAR_PTR(mb_mod[kernel_module].cmdline),
			"[KERNEL]");
      putchar('\n');

      boot_info.kernel_start = load_module(kernel_module,
                                           &boot_info.kernel_low,
                                           &boot_info.kernel_high);
    }

  /* setup sigma0 */
  if (sigma0)
    {
      printf("  Loading ");
      print_module_name(L4_CONST_CHAR_PTR(mb_mod[sigma0_module].cmdline),
			 "[SIGMA0]");
      putchar('\n');

      boot_info.sigma0_start = load_module(sigma0_module,
                                           &boot_info.sigma0_low,
                                           &boot_info.sigma0_high);
      boot_info.sigma0_stack = (l4_addr_t)sigma0_init_stack
                               + sizeof(sigma0_init_stack);
    }

  /* setup roottask */
  if (roottask)
    {

      printf("  Loading ");
      print_module_name(L4_CONST_CHAR_PTR(mb_mod[roottask_module].cmdline),
			 "[ROOTTASK]");
      putchar('\n');

      boot_info.roottask_start = load_module(roottask_module,
                                             &boot_info.roottask_low,
                                             &boot_info.roottask_high);
      boot_info.roottask_stack = (l4_addr_t)roottask_init_stack
	                         + sizeof(roottask_init_stack);
    }

  /* setup kernel PART TWO (special kernel initialization) */
  l4i = find_kip();

#if defined(ARCH_x86) || defined(ARCH_amd64)
  /* setup multi boot info structure for kernel */
  kernel_mbi = *mb_info;
  kernel_mbi.flags = L4UTIL_MB_MEMORY;
  if (mb_mod[kernel_module].cmdline)
    {
      kernel_mbi.cmdline = mb_mod[kernel_module].cmdline;
      kernel_mbi.flags  |= L4UTIL_MB_CMDLINE;
    }

  /* setup the L4 kernel info page before booting the L4 microkernel:
   * patch ourselves into the booter task addresses */
  unsigned long api_version = get_api_version(l4i);
  unsigned major = api_version >> 24;
  printf("  API Version: (%x) %s\n", major, (major & 0x80)?"experimental":"");
  switch (major)
    {
    case 0x02: // Version 2 API
    case 0x03: // Version X.0 and X.1
    case 0x87: // Fiasco V2++
      init_kip_v2(l4i, &boot_info, mb_info);
      break;
    case 0x84:
    case 0x04:
      init_kip_v4(l4i, &boot_info, mb_info);
      break;
    default:
      panic("cannot boot a kernel with unknown api version %lx\n", api_version);
      break;
    }
#endif

  regions_dump();

  printf("  Starting kernel ");
  print_module_name(L4_CONST_CHAR_PTR(mb_mod[kernel_module].cmdline),
		    "[KERNEL]");
  printf(" at "l4_addr_fmt"\n", boot_info.kernel_start);

#if defined(ARCH_x86)
  asm volatile
    ("pushl $exit ; jmp *%3"
     :
     : "a" (L4UTIL_MB_VALID),
       "b" (&kernel_mbi),
       "S" (realmode_or_xen_si_pointer),
       "r" (boot_info.kernel_start));

#elif defined(ARCH_amd64)

  asm volatile
    ("push $exit; jmp *%2"
     :
     : "S" (L4UTIL_MB_VALID), "D" (&kernel_mbi), "r" (boot_info.kernel_start));

#elif defined(ARCH_arm)
  init_kip_arm(l4i, &boot_info, mb_info);
#else

#error "How to enter the kernel?"

#endif

  /*NORETURN*/
}

static int
l4_exec_read_exec(void * handle,
		  l4_addr_t file_ofs, l4_size_t file_size,
		  l4_addr_t mem_addr, l4_size_t mem_size,
		  exec_sectype_t section_type)
{
  exec_task_t *exec_task = (exec_task_t*)handle;

  if (! (section_type & EXEC_SECTYPE_ALLOC))
    return 0;

  if (! (section_type & (EXEC_SECTYPE_ALLOC|EXEC_SECTYPE_LOAD)))
    return 0;

  if (mem_addr < exec_task->begin)
    exec_task->begin = mem_addr;
  if (mem_addr + mem_size > exec_task->end)
    exec_task->end = mem_addr + mem_size;

  //printf("    [%p-%p]\n", (void *) mem_addr, (void *) (mem_addr + mem_size));

  memcpy((void *) mem_addr,
         (char*)(exec_task->mod_start) + file_ofs, file_size);
  if (file_size < mem_size)
    memset((void *) (mem_addr + file_size), 0, mem_size - file_size);


  region_name(mem_addr, mem_addr + mem_size,
             exec_task->mod->cmdline
	       ? L4_CONST_CHAR_PTR(exec_task->mod->cmdline)
	       :  ".[Unknown]");
  return 0;
}

static int
l4_exec_add_region(void * handle,
		  l4_addr_t file_ofs, l4_size_t file_size,
		  l4_addr_t mem_addr, l4_size_t mem_size,
		  exec_sectype_t section_type)
{
  exec_task_t *exec_task = (exec_task_t*)handle;

  if (! (section_type & EXEC_SECTYPE_ALLOC))
    return 0;

  if (! (section_type & (EXEC_SECTYPE_ALLOC|EXEC_SECTYPE_LOAD)))
    return 0;

  region_add(mem_addr, mem_addr + mem_size,
             exec_task->mod->cmdline
	       ? L4_CONST_CHAR_PTR(exec_task->mod->cmdline)
	       :  ".[Unknown]");
  return 0;
}
