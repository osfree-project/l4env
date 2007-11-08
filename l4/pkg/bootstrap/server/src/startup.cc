/* $Id$ */
/**
 * \file	bootstrap/server/src/startup.c
 * \brief	Main functions
 *
 * \date	09/2004
 * \author	Torsten Frenzel <frenzel@os.inf.tu-dresden.de>,
 *		Frank Mehnert <fm3@os.inf.tu-dresden.de>,
 *		Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *		Alexander Warg <aw11@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

/* LibC stuff */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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

static l4util_mb_info_t *mb_info;
/* management of allocated memory regions */
static Region_list regions;
static Region __regs[MAX_REGION];

/* management of conventional memory regions */
static Region_list ram;
static Region __ram[8];

/*
 * IMAGE_MODE means that all boot modules are linked together to one
 * big binary. This mode is in particular usefull for embedded systems.
 */
#ifdef IMAGE_MODE
l4_addr_t _mod_addr = RAM_BASE + MODADDR;
#else
l4_addr_t _mod_addr;
#endif

/* hardware config variables */
int hercules = 0; /* hercules monitor attached */

/* modules to load by bootstrap */
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
extern "C" void startup(l4util_mb_info_t *, l4_umword_t, void *);


static exec_handler_func_t l4_exec_read_exec;
static exec_handler_func_t l4_exec_add_region;



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


/**
 * Scan the memory regions with type == Region::Kernel for a
 * kernel interface page (KIP).
 *
 * After loading the kernel we scan for the magic number at page boundaries.
 */
static
void *find_kip()
{
  enum { L4_KERNEL_INFO_MAGIC = 0x4BE6344CL /* "L4µK" */ };
  unsigned char *p, *end;
  void *k = 0;

  printf("  find kernel info page...\n");
  for (Region const *m = regions.begin(); m != regions.end(); ++m)
    {
      if (m->type() != Region::Kernel)
	continue;

      if (sizeof(unsigned long) < 8
          && m->end() >= (1ULL << 32))
	end = (unsigned char *)(~0UL - 0x1000);
      else
	end = (unsigned char *) (unsigned long)m->end();

      for (p = (unsigned char *) (unsigned long)(m->begin() & 0xfffff000);
	   p < end;
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
    }

  if (!k)
    panic("could not find kernel info page, maybe your kernel is too old");

  return k;
}


/**
 * Get the API version from the KIP.
 */
static inline
unsigned long get_api_version(void *kip)
{
  return ((unsigned long *)kip)[1];
}


/**
 * Scan the command line for the given argument.
 *
 * The cmdline string may either be including the calling program
 * (.../bootstrap -arg1 -arg2) or without (-arg1 -arg2) in the realmode
 * case, there, we do not have a leading space
 *
 * return pointer after argument, NULL if not found
 */
static char *
check_arg(char *cmdline, const char *arg)
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
 * Calculate the maximum memory limit in MB.
 *
 * The limit is the highes physical address where conventional RAM is allowed.
 *
 * If available the '-maxmem=xx' command line option is used.
 * If not then the memory is limited to 3 GB IA32 and unlimited on other
 * systems.
 */
static
unsigned long
get_memory_limit(l4util_mb_info_t *mbi)
{
  char *c;

  /* maxmem= parameter? */
  if ((mbi->flags & L4UTIL_MB_CMDLINE) &&
      (c = check_arg(L4_CHAR_PTR(mbi->cmdline), "-maxmem=")))
    return strtoul(c + 8, NULL, 10) << 20;
  else
#if defined(ARCH_x86)
    /* Limit memory, we cannot really handle more right now. In fact, the
     * problem is roottask. It maps as many superpages/pages as it gets.
     * After that, the remaining pages are mapped using l4sigma0_map_anypage()
     * with a receive window of L4_WHOLE_ADDRESS_SPACE. In response Sigma0
     * could deliver pages beyond the 3GB user space limit. */
    return 3024UL << 20;
#else
    return ~0UL;
#endif
}


/**
 * Add the given memory region as coventional memory.
 *
 * We consider the restricions given on the command line or for the platform
 * while adding regions (i.e., memory beyond the limit from get_memory_limit()
 * will not be added).
 */
static void
add_ram(l4util_mb_info_t *mbi, Region mem)
{
  static unsigned long mem_limit = get_memory_limit(mbi);
  if (mem.invalid())
    {
      printf("  WARNING: trying to add invalid region of conventional RAM.\n");
      return;
    }

  if (mem.begin() >= mem_limit)
    {
      printf("  Dropping RAM region ");
      mem.print();
      printf(" due to %ld MB limit\n", mem_limit >> 20);
      return;
    }

  if (mem.end() >= mem_limit-1)
    {
      printf("  Limiting RAM region ");
      mem.print();
      mem.end(mem_limit-1);
      printf(" to ");
      mem.print();
      printf(" due to %ld MB limit\n", mem_limit >> 20);
    }

  ram.add(mem);
}


/**
 * Move modules to another address.
 *
 * Source and destination regions may overlap.
 */
static void
move_modules(l4util_mb_info_t *mbi, unsigned long modaddr)
{
  long offset = modaddr - (L4_MB_MOD_PTR(mbi->mods_addr))[0].mod_start;
  unsigned i;
  unsigned dir = offset > 0 ? mbi->mods_count : 1;

  if (!offset)
    {
      printf("  => Images in place\n");
      return;
    }

  printf("  move modules to %lx with offset %lx\n", modaddr, offset);

  for (i = dir; i != mbi->mods_count - dir ; offset > 0 ? i-- : i++)
    {
      unsigned long start = (L4_MB_MOD_PTR(mbi->mods_addr))[i-1].mod_start;
      unsigned long end = (L4_MB_MOD_PTR(mbi->mods_addr))[i-1].mod_end;

      if (start == end)
	continue;

      printf("  move module %d start %lx -> %lx\n",i, start, start+offset);
      Region *overlap = regions.find(Region(start + offset, end + offset));
      if (overlap)
	{
	  printf("ERROR: module target [%lx-%lx) overlaps\n", start + offset, 
	      end + offset);
	  overlap->vprint();
	  panic("can not move module\n");
	}
      memmove((void *)(start+offset), (void *)start, end-start);
      (L4_MB_MOD_PTR(mbi->mods_addr))[i-1].mod_start += offset;
      (L4_MB_MOD_PTR(mbi->mods_addr))[i-1].mod_end += offset;
    }
}


/**
 * Add the bootstrap binary itself to the allocated memory regions.
 */
static void
init_regions()
{
  extern int _start;	/* begin of image -- defined in crt0.S */
  extern int _end;	/* end   of image -- defined by bootstrap.ld */

  regions.add(Region::n((unsigned long)&_start, (unsigned long)&_end, 
	".bootstrap", Region::Boot));
}


/**
 * Add the memory containing the boot modules to the allocated regions.
 */
static void
add_boot_modules_region(l4util_mb_info_t *mbi)
{
  regions.add(
      Region::n((L4_MB_MOD_PTR(mbi->mods_addr))[0].mod_start,
	     (L4_MB_MOD_PTR(mbi->mods_addr))[mbi->mods_count-1].mod_end,
	     ".Modules Memory", Region::Root));
}


/**
 * Add all sections of the given ELF binary to the allocated regions.
 * Actually does not load the ELF binary (see load_elf_module()).
 */
static void
add_elf_regions(l4util_mb_info_t *mbi, l4_umword_t module,
                Region::Type type)
{
  exec_task_t exec_task;
  l4_addr_t entry;
  int r;
  const char *error_msg;
  l4util_mb_mod_t *mb_mod = (l4util_mb_mod_t*)mbi->mods_addr;

  assert(module < mbi->mods_count);

  exec_task.begin = 0xffffffff;
  exec_task.end   = 0;
  exec_task.type = type;

  exec_task.mod_start = L4_VOID_PTR(mb_mod[module].mod_start);
  exec_task.mod       = mb_mod + module;

  printf("  Scanning %s\n", L4_CHAR_PTR(mb_mod[module].cmdline));

  r = exec_load_elf(l4_exec_add_region, &exec_task,
                    &error_msg, &entry);

  if (r)
    panic("\nThis is an invalid binary, fix it.");
}


/**
 * Load the given ELF binary into memory and free the source
 * memory region.
 */
static l4_addr_t
load_elf_module(l4util_mb_mod_t *mb_mod)
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
  else
    {
      Region m = Region::n(mb_mod->mod_start, mb_mod->mod_end);
      Region *x = regions.find(m);
      if (x)
	{
	  if (x->begin() == m.begin())
	    {
	      unsigned long b = l4_round_page(m.end()+1);
	      if (x->end() <= b)
		regions.remove(x);
	      else
		x->begin(b);
	    }
	}
    }

  return entry;
}

/**
 * Simple linear memory allocator.
 *
 * Allocate size bytes startting from *ptr and set *ptr to *ptr + size.
 */
static inline void*
lin_alloc(l4_size_t size, char **ptr)
{
  void *ret = *ptr;
  *ptr += (size + 3) & ~3;;
  return ret;
}


/**
 * Duplicate the given command line.
 *
 * This function is use for relocating the multi-boot info.
 * The new location is *ptr and *ptr is incemented by the size of the
 * string (basically like lin_alloc() does).
 *
 * This function also implements the mechanism to replace the command line
 * of a module from the bootstrap comand line.
 */
static
char *dup_cmdline(l4util_mb_info_t *mbi, unsigned mod_nr, char **ptr,
    char const *orig)
{
  char *res = *ptr;
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
 * Search for available memory.
 *
 * @param start The start address for the search.
 * @param end The end address the memory block must not exceed.
 * @param size The size of the block in bytes.
 * @param align The number of least significant bits of the address that 
 *        must be 0.
 *
 * @retrun The address of a free memory region, or 0 if non found.
 *
 * The search is used from Region_list::find_free() and is first fit.
 */
static
unsigned long
find_free_ram(unsigned long start, unsigned long end, unsigned long size,
    unsigned align)
{
  for (Region const *mem = ram.begin(); mem < ram.end(); ++mem)
    {
      Region search = mem->intersect(Region(start,end));
      if (search.invalid())
	continue;

      unsigned long addr = regions.find_free(search, size, align);
      if (addr)
	return addr;
    }
  return 0;
}

static
void
print_e820_map(l4util_mb_info_t *mbi)
{
  printf("  Bootloader MMAP%s\n", mbi->flags & L4UTIL_MB_MEM_MAP
                                   ? ":" : " not available.");

  if (mbi->flags & L4UTIL_MB_MEM_MAP)
    {
      l4util_mb_addr_range_t *mmap;
      for (mmap = (l4util_mb_addr_range_t *) mbi->mmap_addr;
	  (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
	  mmap = (l4util_mb_addr_range_t *) ((unsigned long)mmap + mmap->struct_size + sizeof (mmap->struct_size)))
	{
	  const char *types[] = { "unknown", "RAM", "reserved", "ACPI",
                                  "ACPI NVS", "unusable" };
	  const char *type_str = (mmap->type < (sizeof(types) / sizeof(types[0])))
                                 ? types[mmap->type] : types[0];

	  printf("    [%9llx, %9llx) %s (%d)\n",
                 (unsigned long long) mmap->addr,
                 (unsigned long long) mmap->addr + (unsigned long long) mmap->size,
                 type_str, (unsigned) mmap->type);
	}
    }


}

/**
 * Relocate and compact the multi-boot infomation (MBI).
 *
 * This function relocates the MBI into the first 4MB of physical memory.
 * Substructures such as module information, the VESA information, and
 * the command lines of the modules are also relocated.
 * During relocation of command lines they may be substituted according
 * to '-arg=' options from the bootstrap command line.
 *
 * The memory map is discared and not relocated, because everything after 
 * bootstrap has to use the KIP memory desriptors.
 */
static
l4util_mb_info_t *
relocate_mbi(l4util_mb_info_t *src_mbi, unsigned long* start,
             unsigned long* end)
{
  l4util_mb_info_t *dst_mbi;
  l4_addr_t x;

  print_e820_map(src_mbi);

  x = find_free_ram(RAM_BASE + 0x2000, RAM_BASE + (4 << 20),
      16 << 10, L4_LOG2_PAGESIZE);
  if (!x)
    panic("no memory for mbi found (below 4MB)");

  void *mbi_start = (void*)x;

  char *p = (char*)x;
  *start = x;

  dst_mbi = (l4util_mb_info_t*)lin_alloc(sizeof(l4util_mb_info_t), &p);

  /* copy (extended) multiboot info structure */
  memcpy(dst_mbi, src_mbi, sizeof(l4util_mb_info_t));

  dst_mbi->flags &= ~(L4UTIL_MB_CMDLINE | L4UTIL_MB_MEM_MAP | L4UTIL_MB_MEMORY);

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

  /* copy command lines of modules */
  for (unsigned i = 0; i < dst_mbi->mods_count; i++)
    {
      char *n = dup_cmdline(src_mbi, i, &p, (char const *)(mods[i].cmdline));
      if (n)
	  mods[i].cmdline = (l4_addr_t) n;
    }
  *end = (l4_addr_t)p;

  printf("  Relocated mbi to [%p-%p]\n", mbi_start, (void*)(*end));
  regions.add(Region::n((unsigned long)mbi_start, 
	((unsigned long)*end) + 0xfe,
	".Multiboot info", Region::Root));

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


/**
 * Create the basic multi-boot structure in IMAGE_MODE
 */
static void
construct_mbi(l4util_mb_info_t *mbi)
{
  unsigned i;
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
      printf("  mod%02u: %08x-%08x: %s\n",
	     i, mods[i].mod_start, mods[i].mod_end,
	     L4_CHAR_PTR(_module_info_start[i].name));
      if (image == 0)
	panic("Failure decompressing image\n");

      destbuf += l4_round_page(_module_info_start[i].size_uncompressed);
    }
}
#endif /* IMAGE_MODE */


void
init_pc_serial(l4util_mb_info_t *mbi)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  /* parse command line */
  if (mbi->flags & L4UTIL_MB_CMDLINE)
    {
      const char *s;
      int comport = 1;

      if ((s = check_arg(L4_CHAR_PTR(mbi->cmdline), "-comport")))
	comport = strtoul(s + 9, 0, 0);

      if (check_arg(L4_CHAR_PTR(mbi->cmdline), "-serial"))
	com_cons_init(comport);

      if (check_arg(L4_CHAR_PTR(mbi->cmdline), "-hercules") && have_hercules())
        hercules = 1;
    }
#endif

}


/**
 * Read MBI memory map, or basic memory information if MMAP is not available
 * and initialize the 'ram' and 'regions' lists.
 */
void
init_memory_map(l4util_mb_info_t *mbi)
{
  if (!(mbi->flags & L4UTIL_MB_MEM_MAP))
    {
      assert(mbi->flags & L4UTIL_MB_MEMORY);
      add_ram(mbi, Region::n(0, (mbi->mem_upper + 1024) << 10, ".ram",
              Region::Ram));
    }
  else
    {
      for (l4util_mb_addr_range_t *mmap
            = (l4util_mb_addr_range_t *)mbi->mmap_addr;
           (unsigned long)mmap < mbi->mmap_addr + mbi->mmap_length;
           mmap = (l4util_mb_addr_range_t *) ((unsigned long) mmap
            + mmap->struct_size + sizeof (mmap->struct_size)))
      {

	unsigned long long start = (unsigned long long)mmap->addr;
	unsigned long long end = (unsigned long long)mmap->addr + mmap->size;

	switch (mmap->type)
	  {
	  case 1:
	    add_ram(mbi, Region::n(start, end, ".ram", Region::Ram));
	    break;
	  case 2:
	  case 3:
	  case 4:
	    regions.add(Region::n(start, end, ".BIOS", Region::Arch, mmap->type));
	    break;
	  case 5:
	    regions.add(Region::n(start, end, ".BIOS", Region::No_mem));
	    break;
	  default:
	    break;
	  }
      }
    }
}


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

  /* fire up serial port if specificed on the command line */
  init_pc_serial(mbi);

  puts("\nL4 Bootstrapper");

  regions.init(__regs, sizeof(__regs)/sizeof(__regs[0]));
  ram.init(__ram, sizeof(__ram)/sizeof(__ram[0]));

#if defined(ARCH_x86) || defined(ARCH_amd64)

#ifdef REALMODE_LOADING
  /* create synthetic multi bott info */
  mbi = init_loader_mbi(realmode_si, 0);
#else
  assert(flag == L4UTIL_MB_VALID); /* we need to be multiboot-booted */
#endif

  /* read MMAP info from the MBI if available */
  init_memory_map(mbi);

#else /* arm */
#ifdef ARCH_arm
  l4util_mb_info_t my_mbi;
  memset(&my_mbi, 0, sizeof(my_mbi));
  mbi = &my_mbi;
  printf("  Memory size is %dMB\n", RAM_SIZE_MB);
  add_ram(mbi, Region::n(RAM_BASE,
          (unsigned long long)RAM_BASE + (RAM_SIZE_MB << 20), ".ram",
          Region::Ram));
#else
#error Unknown arch!
#endif
#endif

  /* basically add the bootstrap binary to the allocated regions */
  init_regions();

  /* parse command line */
  if (mbi->flags & L4UTIL_MB_CMDLINE)
    {
      const char *s;

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
  /* We have at least the L4 kernel and the first user task */
  assert(mbi->mods_count >= 2);
  assert(mbi->mods_count <= MODS_MAX);

  /* we're just a GRUB-booted kernel! */
  add_boot_modules_region(mbi);

  if (mbi->flags & L4UTIL_MB_CMDLINE)
    {
      const char *s;
      /* patch modules with content given at command line */
      s = L4_CONST_CHAR_PTR(mbi->cmdline);
      while ((s = check_arg((char *)s, "-patch=")))
	patch_module(&s, mbi);
    }


  add_elf_regions(mbi, kernel_module, Region::Kernel);

  if (sigma0)
    add_elf_regions(mbi, sigma0_module, Region::Sigma0);

  if (roottask)
    add_elf_regions(mbi, roottask_module, Region::Root);


  /* copy Multiboot data structures, we still need to a safe place
   * before playing with memory we don't own and starting L4 */
  mb_info = relocate_mbi(mbi, &boot_info.mbi_low, &boot_info.mbi_high);
  if (!mb_info)
    panic("could not copy multiboot info to memory below 4MB");

  mb_mod = (l4util_mb_mod_t*)mb_info->mods_addr;

  /* --- Shouldn't touch original Multiboot parameters after here. -- */

  /* setup kernel PART ONE */
  printf("  Loading ");
  print_module_name(L4_CONST_CHAR_PTR(mb_mod[kernel_module].cmdline),
      "[KERNEL]");
  putchar('\n');

  boot_info.kernel_start = load_elf_module(mb_mod + kernel_module);

  /* setup sigma0 */
  if (sigma0)
    {
      printf("  Loading ");
      print_module_name(L4_CONST_CHAR_PTR(mb_mod[sigma0_module].cmdline),
			 "[SIGMA0]");
      putchar('\n');

      boot_info.sigma0_start = load_elf_module(mb_mod + sigma0_module);
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

      boot_info.roottask_start = load_elf_module(mb_mod + roottask_module);
      boot_info.roottask_stack = (l4_addr_t)roottask_init_stack
	                         + sizeof(roottask_init_stack);
    }

  /* setup kernel PART TWO (special kernel initialization) */
  l4i = find_kip();

#if defined(ARCH_x86) || defined(ARCH_amd64)
  /* setup multi boot info structure for kernel */
  l4util_mb_info_t kernel_mbi;
  kernel_mbi = *mb_info;
  kernel_mbi.flags = L4UTIL_MB_MEMORY;
  if (mb_mod[kernel_module].cmdline)
    {
      kernel_mbi.cmdline = mb_mod[kernel_module].cmdline;
      kernel_mbi.flags  |= L4UTIL_MB_CMDLINE;
    }
#endif

  regions.optimize();
  regions.dump();

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
      init_kip_v2(l4i, &boot_info, mb_info, &ram, &regions);
      break;
    case 0x84:
    case 0x04:
      init_kip_v4(l4i, &boot_info, mb_info, &ram, &regions);
      break;
    default:
      panic("cannot boot a kernel with unknown api version %lx\n", api_version);
      break;
    }

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
  typedef void (*startup_func)(void);
  startup_func f = (startup_func)boot_info.kernel_start;
  f();
#else

#error "How to enter the kernel?"

#endif

  /*NORETURN*/
}

static int
l4_exec_read_exec(void * handle,
		  l4_addr_t file_ofs, l4_size_t file_size,
		  l4_addr_t mem_addr, l4_addr_t /*v_addr*/,
		  l4_size_t mem_size,
		  exec_sectype_t section_type)
{
  exec_task_t *exec_task = (exec_task_t*)handle;
  if (!mem_size)
    return 0;

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


  Region *f = regions.find(mem_addr);
  if (!f)
    {
      printf("could not find %lx\n", mem_addr);
      regions.dump();
      panic("Oops: region for module not found\n");
    }

  f->name(exec_task->mod->cmdline
	       ? L4_CONST_CHAR_PTR(exec_task->mod->cmdline)
	       :  ".[Unknown]");
  return 0;
}

static int
l4_exec_add_region(void * handle,
		  l4_addr_t file_ofs, l4_size_t file_size,
		  l4_addr_t mem_addr, l4_addr_t v_addr,
		  l4_size_t mem_size,
		  exec_sectype_t section_type)
{
  exec_task_t *exec_task = (exec_task_t*)handle;

  if (!mem_size)
    return 0;

  if (! (section_type & EXEC_SECTYPE_ALLOC))
    return 0;

  if (! (section_type & (EXEC_SECTYPE_ALLOC|EXEC_SECTYPE_LOAD)))
    return 0;

  regions.add(Region::n(mem_addr, mem_addr + mem_size,
             exec_task->mod->cmdline
	       ? L4_CONST_CHAR_PTR(exec_task->mod->cmdline)
	       :  ".[Unknown]", Region::Type(exec_task->type),
	       mem_addr == v_addr ? 1 : 0));
  return 0;
}
