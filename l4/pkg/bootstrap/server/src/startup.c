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
#include "loader_mbi.h"
#if defined (ARCH_x86) || defined(ARCH_amd64)
#include "ARCH-x86/serial.h"
#endif


#undef getchar

static char bootstrap_mbi[16 << 10];

#ifdef IMAGE_MODE
l4_addr_t _mod_addr = MODADDR;
#else
l4_addr_t _mod_addr;
#endif

/* hardware config variables */
int hercules = 0; /* hercules monitor attached */

/* modules to load by bootstrap */
static const int kernel = 1; /* we have at least a kernel */
static int sigma0 = 1;   /* we need sigma0 */
static int roottask = 1; /* we need a roottask */

enum {
  kernel_module,
  sigma0_module,
  roottask_module,
};

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
find_kip(boot_info_t *bi)
{
  enum { L4_KERNEL_INFO_MAGIC = 0x4BE6344CL /* "L4µK" */ };
  unsigned char *p;
  void *k = 0;

  printf("  find kernel info page...\n");
  for (p = (unsigned char *) (bi->kernel_low & 0xfffff000);
       p < (unsigned char *) bi->kernel_low +
         (bi->kernel_high - bi->kernel_low);
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
 * The cmdline string may either be including the calling program
 * (.../bootstrap -arg1 -arg2) or without (-arg1 -arg2) in the realmode
 * case, there, we do not have a leading space
 *
 * return pointer after argument, NULL if not found
 */
static char *
check_arg(char *cmdline, char *arg)
{
  char *s = cmdline;
  while ((s = strstr(s, arg)))
    {
      if (s == cmdline
          || isspace(s[-1]))
        return s;
    }

  return NULL;
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
      (c = check_arg(L4_CHAR_PTR(mbi->cmdline), "-maxmem=")))
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
 * Move modules to another address.
 */
static void
move_modules(l4util_mb_info_t *mbi, unsigned modaddr)
{
  int offset = modaddr - (L4_MB_MOD_PTR(mbi->mods_addr))[0].mod_start;
  unsigned i;
  unsigned dir = offset > 0 ? mbi->mods_count : 1;

  if (!offset)
    {
      printf(" => Images in place\n");
      return;
    }

  printf("  move modules to %x with offset %x\n", modaddr, offset);

  for (i = dir; i != mbi->mods_count - dir ; offset > 0 ? i-- : i++)
    {
      unsigned start = (L4_MB_MOD_PTR(mbi->mods_addr))[i-1].mod_start;
      unsigned end = (L4_MB_MOD_PTR(mbi->mods_addr))[i-1].mod_end;

      printf("  move module %d start %x -> %x\n",i, start, start+offset);
      if ((start+offset < 0x100000) || (end+offset) >> 10 > mbi->mem_upper)
	panic("can not move module to [%x,%x]\n", start+offset, end+offset);
      memmove((void *)(start+offset), (void *)start, end-start);
      (L4_MB_MOD_PTR(mbi->mods_addr))[i-1].mod_start += offset;
      (L4_MB_MOD_PTR(mbi->mods_addr))[i-1].mod_end += offset;
    }
}


/**
 * init region map
 */
static void
init_region(l4util_mb_info_t *mbi, boot_info_t *bi)
{
  extern int _start;	/* begin of image -- defined in crt0.S */
  extern int _end;	/* end   of image -- defined by bootstrap.ld */


#if defined(ARCH_x86) || defined(ARCH_amd64)
  region_add(0x09f000, 0x100000, ".BIOS area");
#endif  

  region_add((l4_addr_t)&_start, (l4_addr_t)&_end, ".bootstrap");

  region_add((L4_MB_MOD_PTR(mbi->mods_addr))[0].mod_start,
	     (L4_MB_MOD_PTR(mbi->mods_addr))[mbi->mods_count-1].mod_end,
	     ".Modules Memory");

  // RAM memory size
  region_add(0,               RAM_BASE,      ".Deep Space");
  region_add(RAM_BASE + bi->mem_high, ~0U,   ".Deep Space");
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
  exec_task.mod       = mb_mod + module;

  printf("  Scanning %s\n", L4_CHAR_PTR(mb_mod[module].cmdline));

  r = exec_load_elf(l4_exec_add_region, &exec_task,
                    &error_msg, &entry);
}

static l4_addr_t
load_module(l4util_mb_mod_t *mb_mod, l4_addr_t *begin, l4_addr_t *end)
{
  exec_task_t exec_task;
  l4_addr_t entry;
  int r;
  const char *error_msg;

  exec_task.begin = 0xffffffff;
  exec_task.end   = 0;

  exec_task.mod_start = L4_VOID_PTR(mb_mod->mod_start);
  exec_task.mod       = mb_mod;

  r = exec_load_elf(l4_exec_read_exec, &exec_task,
                    &error_msg, &entry);

#ifndef RELEASE_MODE
  /* clear the image for debugging and security reasons */
  memset(L4_VOID_PTR(mb_mod->mod_start), 0,
         mb_mod->mod_end - mb_mod->mod_start);
#endif

  if (r)
    printf("  => can't load module (%s)\n", error_msg);

  *begin = exec_task.begin;
  *end   = exec_task.end;

  /* Only add region if non-elf module */
  if (r)
    region_add(exec_task.begin, exec_task.end,
	       L4_CHAR_PTR(mb_mod->cmdline));

  return entry;
}


static
unsigned char *
dup_cmdline(l4util_mb_info_t *mbi, unsigned mod_nr, l4_uint8_t **ptr, char const *orig)
{
  unsigned char *res = *ptr;
  if (!orig)
    return 0;

  char* name_end = strchr(orig, ' ');
  if (name_end && *name_end)
    *name_end = 0;
  else
    name_end = 0;

  unsigned size;
  char const *new_args = get_arg_module(mbi, orig, &size);

  if (new_args && size)
    printf("    new args for %d = \"%.*s\"\n", mod_nr, size, new_args);
  else
    if (name_end)
      *name_end = ' ';

  strcpy(*ptr, orig);
  *ptr+= strlen(*ptr)+1;

  if (new_args)
    {
      *((*ptr)-1) = ' ';
      strncpy(*ptr, new_args, size);
      *ptr += size;
      *((*ptr)++) = 0;
    }
  return res;
}


/**
 * alloc linear pointer from an backing store array.
 */
static inline void*
lin_alloc(l4_size_t size, l4_uint8_t **ptr, l4_size_t *free)
{
  void *ret = *ptr;
  *ptr += (size + 3) & ~3;
  assert(*free > (size + 3));
  *free -= (size + 3) & ~3;
  return ret;
}


/**
 * copy mbi to save place for boot
 */
static
void *
relocate_mbi(l4util_mb_info_t *src_mbi, void *mbi_start,
             l4_size_t free)
{
  int i;
  l4util_mb_info_t *dst_mbi;
  l4_uint8_t *p = mbi_start;
  dst_mbi = lin_alloc(sizeof(l4util_mb_info_t), &p, &free);
  printf("  Bootloader MMAP %s\n", src_mbi->flags & L4UTIL_MB_MEM_MAP ? "available" : "not available");

  l4util_mb_addr_range_t *mmap;
  for (mmap = (l4util_mb_addr_range_t *) src_mbi->mmap_addr;
       (unsigned long) mmap < src_mbi->mmap_addr + src_mbi->mmap_length;
       mmap = (l4util_mb_addr_range_t *) ((unsigned long) mmap + mmap->struct_size + sizeof (mmap->struct_size)))
    {
      char *types[] = { "unknown", "RAM", "reserved", "ACPI", "ACPI NVS", "unusable" };
      char *type_str = (mmap->type < (sizeof(types) / sizeof(types[0])))
	               ? types[mmap->type] : types[0];

      printf("    [%08x,%08x) %s (%d)\n",
             (unsigned) mmap->addr, (unsigned) mmap->addr + (unsigned) mmap->size,
             type_str, (unsigned) mmap->type);
    }

  /* copy (extended) multiboot info structure */
  memcpy(dst_mbi, src_mbi, sizeof(l4util_mb_info_t));

  if ((src_mbi->flags & L4UTIL_MB_CMDLINE) && src_mbi->cmdline)
    {
      void *dest = lin_alloc(strlen((void *)src_mbi->cmdline) + 1, &p, &free);
      strcpy(dest, (void *)src_mbi->cmdline);
      dst_mbi->cmdline = (unsigned long)dest;
	  
    }

  if (dst_mbi->flags & L4UTIL_MB_MEM_MAP)
    {
      if (src_mbi->mmap_addr)
	{
	  void *mmap = lin_alloc(src_mbi->mmap_length, &p, &free);
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
            = lin_alloc(sizeof(l4util_mb_vbe_mode_t), &p, &free);
	  memcpy(m, L4_VOID_PTR(src_mbi->vbe_mode_info),
		 sizeof(l4util_mb_vbe_mode_t));
	  dst_mbi->vbe_mode_info = (l4_addr_t)m;
	}

      /* copy VBE controller info structure */
      if (src_mbi->vbe_ctrl_info)
	{
	  l4util_mb_vbe_ctrl_t *m
            = lin_alloc(sizeof(l4util_mb_vbe_ctrl_t), &p, &free);
	  memcpy(m, L4_VOID_PTR(src_mbi->vbe_ctrl_info),
		 sizeof(l4util_mb_vbe_ctrl_t));
	  dst_mbi->vbe_ctrl_info = (l4_addr_t)m;
	}
    }

  /* copy module descriptions */
  l4util_mb_mod_t *mods
    = lin_alloc(sizeof(l4util_mb_mod_t) * src_mbi->mods_count, &p, &free);
  memcpy(mods, L4_VOID_PTR(dst_mbi->mods_addr),
	 dst_mbi->mods_count * sizeof (l4util_mb_mod_t));
  dst_mbi->mods_addr = (l4_addr_t)mods;

  /* copy command lines of modules */
  for (i = 0; i < dst_mbi->mods_count; i++)
    {
      unsigned char *n = dup_cmdline(src_mbi, i, &p, (char const *)(mods[i].cmdline));
      if (n)
	  mods[i].cmdline = (l4_addr_t) n;
    }

  printf("  Relocated mbi to [%p-%p]\n", mbi_start, p);
  return p;
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
  l4_addr_t destbuf;

  mbi->mods_count  = _module_info_end - _module_info_start;
  mbi->flags      |= L4UTIL_MB_MODS;
  mbi->mods_addr   = (l4_addr_t)mods;

  assert(mbi->mods_count >= 2);

  destbuf = l4_round_page(mods[mbi->mods_count - 1].mod_end);
  if (destbuf < _mod_addr)
    destbuf = _mod_addr;

  for (i = 0; i < mbi->mods_count; i++)
    {
#ifdef COMPRESS
      l4_addr_t image =
	 (l4_addr_t)decompress(L4_CONST_CHAR_PTR(_module_info_start[i].name),
                               L4_VOID_PTR(_module_info_start[i].start),
                               (void *)destbuf,
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

      destbuf += l4_round_page(_module_info_start[i].size_uncompressed);
    }
}
#endif /* IMAGE_MODE */

/**
 * \brief  Startup, started from crt0.S
 */
void
startup(l4util_mb_info_t *mbi, l4_umword_t flag,
	void *realmode_si)
{
  void *l4i;
  boot_info_t boot_info;
  l4util_mb_mod_t *mb_mod;

#if defined(ARCH_x86) || defined(ARCH_amd64)
  l4util_mb_info_t kernel_mbi;
#endif

#if defined(ARCH_x86) || defined(ARCH_amd64)
#ifdef REALMODE_LOADING
  mbi = init_loader_mbi(realmode_si, 0);
#else
  assert(flag == L4UTIL_MB_VALID); /* we need to be multiboot-booted */
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
      const char *s;
#if defined(ARCH_x86) || defined(ARCH_amd64)
      int comport = 1;

      if ((s = check_arg(L4_CHAR_PTR(mbi->cmdline), "-comport")))
	comport = strtoul(s + 9, 0, 0);

      if (check_arg(L4_CHAR_PTR(mbi->cmdline), "-serial"))
	com_cons_init(comport);

      if (check_arg(L4_CHAR_PTR(mbi->cmdline), "-hercules") && have_hercules())
        hercules = 1;
#endif

      if (check_arg(L4_CHAR_PTR(mbi->cmdline), "-no-sigma0"))
	sigma0 = 0;

      if (check_arg(L4_CHAR_PTR(mbi->cmdline), "-no-roottask"))
	roottask = 0;

      if ((s = check_arg(L4_CHAR_PTR(mbi->cmdline), "-modaddr")))
        _mod_addr = strtoul(s + 9, 0, 0);
    }

#ifdef IMAGE_MODE
  construct_mbi(mbi);
#endif
  if (_mod_addr)
    move_modules(mbi, _mod_addr);

  /* We need at least two boot modules */
  assert(mbi->flags & L4UTIL_MB_MODS);
  assert(mbi->flags & L4UTIL_MB_MEMORY);
  /* We have at least the L4 kernel and the first user task */
  assert(mbi->mods_count >= 2);
  assert(mbi->mods_count <= MODS_MAX);

  /* we're just a GRUB-booted kernel! */
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

  if (mbi->flags & L4UTIL_MB_CMDLINE)
    {
      const char *s;
      /* patch modules with content given at command line */
      s = L4_CONST_CHAR_PTR(mbi->cmdline);
      while ((s = check_arg((char *)s, "-patch=")))
	patch_module(&s, mbi);
    }

  // clear boot_info data structure
  memset(&boot_info, 0, sizeof boot_info);

  /* copy Multiboot data structures, we still need to a safe place
   * before playing with memory we don't own and starting L4 */
  boot_info.mbi_low = (unsigned long)bootstrap_mbi;
  boot_info.mbi_high = (unsigned long)relocate_mbi(mbi, bootstrap_mbi, sizeof(bootstrap_mbi));
  mbi = (l4util_mb_info_t *)bootstrap_mbi;

  /* Guess the amount of installed memory from the bootloader. We already
   * need that value because (1) we have to initialize the memmap for module
   * loading and (2) we have to check for a valid L4 kernel. Later (in init.c),
   * mem_high is determined from the sigma0 server. */
  setup_memory_range(mbi, &boot_info);

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

  mb_mod = (l4util_mb_mod_t*)mbi->mods_addr;

  /* --- Shouldn't touch original Multiboot parameters after here. -- */

  /* setup kernel PART ONE */
  if (kernel)
    {
      printf("  Loading ");
      print_module_name(L4_CONST_CHAR_PTR(mb_mod[kernel_module].cmdline),
			"[KERNEL]");
      putchar('\n');

      boot_info.kernel_start = load_module(mb_mod + kernel_module,
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

      boot_info.sigma0_start = load_module(mb_mod + sigma0_module,
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

      boot_info.roottask_start = load_module(mb_mod + roottask_module,
                                             &boot_info.roottask_low,
                                             &boot_info.roottask_high);
      boot_info.roottask_stack = (l4_addr_t)roottask_init_stack
	                         + sizeof(roottask_init_stack);
    }

  /* setup kernel PART TWO (special kernel initialization) */
  l4i = find_kip(&boot_info);

#if defined(ARCH_x86) || defined(ARCH_amd64)
  /* setup multi boot info structure for kernel */
  kernel_mbi = *mbi;
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
      init_kip_v2(l4i, &boot_info, mbi);
      break;
    case 0x84:
    case 0x04:
      init_kip_v4(l4i, &boot_info, mbi);
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
       "S" (realmode_si),
       "r" (boot_info.kernel_start));

#elif defined(ARCH_amd64)

  asm volatile
    ("push $exit; jmp *%2"
     :
     : "S" (L4UTIL_MB_VALID), "D" (&kernel_mbi), "r" (boot_info.kernel_start));

#elif defined(ARCH_arm)
  init_kip_arm(l4i, &boot_info, mbi);
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
