/* startup stuff */

/* it should be possible to throw away the text/data/bss of the object
   file resulting from this source -- so, we don't define here
   anything we could still use at a later time.  instead, globals are
   defined in globals.c */

/* LibC stuff */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* L4 stuff */
#include <l4/sys/compiler.h>
#include <l4/sys/kernel.h>
#include <l4/sys/kdebug.h>
#include <l4/util/mb_info.h>
#include <l4/exec/elf.h>

#ifndef USE_OSKIT
#include <l4/env_support/getchar.h>
#include <l4/env_support/panic.h>
#endif

/* local stuff */
#include "exec.h"
#ifdef ARCH_x86
#include "ARCH-x86/serial.h"
#endif
#include "version.h"
#include "region.h"
#include "module.h"
#include "startup.h"
#include "init.h"

#if defined(REALMODE_LOADING) || defined(XEN) || defined(ARCH_arm)
#include "loader_mbi.h"
#endif

// Uh
#define MEMORY 64

extern int _start;	/* begin of image -- defined in crt0.S */
extern int _stack;	/* begin of stack -- defined by crt0.S */
extern int _end;	/* end   of image -- defined by bootstrap.ld */

/* temporary buffer for multiboot info */
static l4util_mb_info_t mb_info;
static char mb_cmdline[CMDLINE_MAX];
       l4util_mb_mod_t mb_mod[MODS_MAX];
static char mb_mod_names[2048];
static l4util_mb_vbe_ctrl_t mb_vbe_ctrl;
static l4util_mb_vbe_mode_t mb_vbe_mode;

/* pointers to copy multiboot info to roottask */
static l4util_mb_info_t *mb_info_addr = 0;
static char *mb_cmdline_addr = 0;
static l4util_mb_mod_t *mb_mod_addr = 0;
static char *mb_mod_names_addr = 0;
static l4util_mb_vbe_ctrl_t *mb_vbe_ctrl_addr;
static l4util_mb_vbe_mode_t *mb_vbe_mode_addr;

const unsigned long _mod_addr = MODADDR;

/* hardware config variables */
int hercules = 0; /* hercules monitor attached */

/* kernel stuff variables */
static int l4_version = 0; /* L4 version number */
static int abi_version = 0; /* ABI version number/for v4 kip initialisation */

/* modules to load by bootstrap */
static int kernel = 1; /* we have at least a kernel */
#ifdef XEN
static int sigma0 = 0; /* we need sigma0 */
static int roottask = 0; /* we need a roottask */
#else
static int sigma0 = 1; /* we need sigma0 */
static int roottask = 1; /* we need a roottask */
#endif

enum {
  kernel_module,
  sigma0_module,
  roottask_module,
};

/* RAM memory size */
static l4_addr_t mem_lower = 0;	/* lower bound of physical memory */
static l4_addr_t mem_upper = 0;	/* upper bound of physical memory */
static l4_addr_t mem_high = 0;	/* highest usable address */

static l4_addr_t mod_range_start = 0;
static l4_addr_t mod_range_end = 0;

static l4_addr_t bios_area_start = 0;
static l4_addr_t bios_area_end = 0;

static l4_addr_t bootstrap_start = 0;
static l4_addr_t bootstrap_end = 0;

/* kernel memmory*/
       l4_addr_t kernel_low = 0;
       l4_addr_t kernel_high = 0;
static l4_addr_t kernel_start = 0;

/* sigma0 memmory */
static l4_addr_t sigma0_low = 0;
static l4_addr_t sigma0_high = 0;
static l4_addr_t sigma0_start = 0;
static l4_addr_t sigma0_stack = 0;

/* roottask memmory */
static l4_addr_t roottask_low = 0;
static l4_addr_t roottask_high = 0;
static l4_addr_t roottask_start = 0;
static l4_addr_t roottask_stack = 0;

/* we define a small stack for sigma0 and roottask here --
   it is used by L4 for parameter passing. however, sigma0 and
   roottask must switch to its own private stack as soon as it
   has initialized itself
   because this memory region is later recycled in init.c */
static char roottask_init_stack[64]; /* XXX hardcoded */
static char sigma0_init_stack[64]; /* XXX hardcoded */

/* entry point */
void startup(l4util_mb_info_t *, unsigned int, void *);

static exec_read_func_t l4_exec_read;
static exec_read_exec_func_t l4_exec_read_exec;

/*****************************************************************************/
/**
 * \brief  Detect L4 Version
 *
 * \return L4 Version:
 *         - #VERSION_L4_V2    old GMD L4 version
 *         - #VERSION_L4_IBM   IBM LN/X version
 *         - #VERSION_FIASCO   Fiasco
 *         - #VERSION_L4_KA    L4-KA/Hazelnut
 */
/*****************************************************************************/
static int
check_l4_version(unsigned char * start, unsigned char * end, int * abi_version)
{
  unsigned char *p;

  printf("  Kernel at cheesy %08x-%08x\n", (unsigned)start, (unsigned)end);

  for (p = start; p < end; p++)
    {
      if (memcmp(p, "L4/486 \346-Kernel", 15) == 0)
	{
	  int m4_ok;

	  printf("  Detected L4/486\n");
	  /* remove old 4M event */
	  m4_ok = 0;
	  for (p=start; p<end; p++)
	    {
	      if (memcmp(p, "\314\353\0024M", 5) == 0)
		{
		  p[0] = 0x90; /* replace "int $3" by "nop" */
		  m4_ok=1;
		  break;
		}
	    }
	  if (!m4_ok)
	    printf("  4M sequence not found in L4/486 -- that's OK.\n");
	  return VERSION_L4_V2;
	}

      if (memcmp(p, "L4/Pentium \346-Kernel", 19) == 0)
	{
	  /* it's Jochen's Pentium version */
	  printf("  Detected L4/Pentium\n");
	  return VERSION_L4_V2;
	}

      if (memcmp(p, "Lava Nucleus (Pentium)", 22) == 0)
	{
	  /* it's the IBM version */
	  printf("  Detected IBM LN/Pentium\n");
	  return VERSION_L4_IBM;
	}

      if (memcmp(p, "L4-X Nucleus (x86)", 18) == 0)
	{
	  /* it's the IBM X version */
	  printf("  Detected IBM X/Pentium\n");
#if !defined(L4_API_L4X0) && !defined(L4API_l4x0)
	  /* X.0 API, but not adaption enabled, fail */
	  panic("L4-X uses 32 Bit TIDs, but not adaption enabled");
#endif
	  return VERSION_L4_IBM;
	}

      if (memcmp(p, "DD-L4(v2)/x86 microkernel", 25) == 0
	  || memcmp(p, "DD-L4/x86 microkernel", 21) == 0)
	{
	  /* it's the Dresden version */
	  printf("  Detected new-style DD-L4(v2)/Fiasco x86\n");
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	  panic("Fiasco uses 64 Bit TIDs, but 32 Bit TID adaption enabled");
#endif
	  return VERSION_FIASCO;
	}

      if (memcmp(p, "DD-L4(x0)/x86 microkernel", 25) == 0)
	{
	  /* it's the Dresden version */
	  printf("  Detected new-style DD-L4(x0)/Fiasco x86\n");
#if !defined(L4_API_L4X0) && !defined(L4API_l4x0)
	  panic("Fiasco(x0) uses 32 Bit TIDs, but 64 Bit TID are used");
#endif
	  return VERSION_FIASCO;
	}

      if (memcmp(p, "DD-L4(v4)/x86 microkernel", 25) == 0)
	{
	  /* it's the Dresden version */
	  printf("  Detected very-new-style DD-L4(v4)/Fiasco\n");

	  *abi_version = VERSION_ABI_V4;
	  return VERSION_FIASCO;
	}

      if (memcmp(p, "DD-L4(x0)/arm microkernel", 25) == 0)
	{
	  printf("  Detected DD-L4(x0)/Fiasco ARM\n");
	  return VERSION_FIASCO;
	}

      if (memcmp(p, "L4/KA", 5) == 0 ||
	  memcmp(p, "L4Ka/Hazelnut", 13) == 0)
        {
	  /* it's the Karlsruhe version */
	  printf("  Detected L4Ka/Hazelnut\n");
#if !defined(L4_API_L4X0) && !defined(L4API_l4x0)
	  /* X.0 API, but not adaption enabled, fail */
	  panic("Hazelnut uses 32 Bit TIDs, but not adaption enabled");
#endif
	  return VERSION_L4_KA;
        }

      if (memcmp(p, "L4Ka::Pistachio ", 16) == 0)
        {
	  printf("  Detected L4Ka::Pistachio\n");

	  *abi_version = VERSION_ABI_V4;
	  return VERSION_L4_KA;
	}

    }

  printf("  could not detect version of L4 -- assuming Jochen.\n");
  return VERSION_L4_V2;
}

/*****************************************************************************/
/**
 * \brief  Init Fiasco
 */
/*****************************************************************************/
static void
init_fiasco(l4_kernel_info_t ** l4i)
{
  static char cmdline[256];
  int l;

  /* use an info page prototype allocated from our address space;
     this will be copied later into the kernel's address space */
  static char proto[L4_PAGESIZE] __attribute__((aligned(4096)));

  memset(proto, 0, L4_PAGESIZE);
  *l4i = (l4_kernel_info_t *) proto;

  /* append address of kernel info prototype to kernel's command
     line */
  if (mb_mod[0].cmdline)
    {
      l = strlen((char *) mb_mod[0].cmdline);
      /* make sure it fits into cmdline[] */
      assert(l < sizeof(cmdline) - 40);
      strcpy(cmdline, (char *) mb_mod[0].cmdline);
    }
  else
    l = 0;

#ifdef ARCH_x86
  sprintf(cmdline + l, " proto=0x%x", (l4_addr_t) proto);
#endif
  mb_mod[0].cmdline = (l4_addr_t) cmdline;
}

#ifdef ARCH_x86
/**
 * @brief Initialize KIP prototype for Fiasco/v4.
 */
static void
init_v4_kip (l4_kernel_info_t * l4i)
{
  unsigned *p = (unsigned*) ((char*) l4i + V4KIP_MEM_DESC);
  unsigned *p_meminfo = (unsigned*) ((char*) l4i + V4KIP_MEM_INFO);

  // reserve sigma0 memory
  *(p++) = l4i->sigma0_memory.low | 3;
  *(p++) = l4i->sigma0_memory.high;

  // reserve roottask memory
  *(p++) = l4i->root_memory.low | 3;
  *(p++) = l4i->root_memory.high;

  // set the mem_info variable: offset and count of mem descriptors
  *p_meminfo = (((unsigned) V4KIP_MEM_DESC) << 16)   // offset
    | 2;                                             // count
}
#endif

/*****************************************************************************/
/*
 * setup memory range
 */
/*****************************************************************************/
static void
setup_memory_range(l4util_mb_info_t *mbi)
{
  if (mbi->flags & L4UTIL_MB_MEMORY)
    {
      mem_upper = mbi->mem_upper;
      mem_lower = mbi->mem_lower;
    }

  /* maxmem= parameter? */
  if ((mbi->flags & L4UTIL_MB_CMDLINE)
      && strstr((char *) mbi->cmdline, " -maxmem="))
    {
      mem_upper = 1024 *
	(strtoul((strstr((char *) mbi->cmdline, " -maxmem=") +
		  strlen(" -maxmem=")), NULL, 10) - 1);
    }

  if (mbi->mem_upper != mem_upper)
    printf("  Limiting memory to %d MB (%d KB)\n",
	mem_upper/1024+1, mem_upper+1024);

  mem_high = (mb_info.mem_upper + 1024) << 10;

  /* ensure that we have a valid multiboot info structure */
  mbi->mem_upper = mem_upper;
  mbi->mem_lower = mem_lower;
  mbi->flags     |= L4UTIL_MB_CMDLINE;
}

/*****************************************************************************/
/*
 * init memory map
 */
/*****************************************************************************/
static void
init_memmap(l4util_mb_info_t *mbi)
{
#ifdef ARCH_x86
  bios_area_start = (mbi->mem_lower << 10);
  bios_area_end   = 1024            << 10;
#endif

  bootstrap_start = (unsigned)&_start;
  bootstrap_end   = (unsigned)&_end;

  mod_range_start = ((l4util_mb_mod_t*)(mbi->mods_addr))[0].mod_start;
  mod_range_end   = ((l4util_mb_mod_t*)(mbi->mods_addr))[mbi->mods_count-1].mod_end;
}


/*****************************************************************************/
/*
 * init region map
 */
/*****************************************************************************/
static void
init_region(l4util_mb_info_t *mbi)
{
  region_add(bios_area_start, bios_area_end, ".BIOS area");
  region_add(bootstrap_start, bootstrap_end, ".bootstrap");
  region_add(mod_range_start, mod_range_end, ".Modules Memory");
  region_add(0,               RAM_BASE,      ".Deep Space");
#ifndef XEN
  region_add(RAM_BASE + mem_high, ~0U,       ".Deep Space");
#endif
}

#ifdef ARCH_x86
/*****************************************************************************/
/*
 * setup Kernel Info Page
 */
/*****************************************************************************/
static void
init_kip(l4_kernel_info_t *l4i)
{
  l4i->root_esp = (l4_umword_t) &_stack;

  if (l4_version != VERSION_FIASCO)
    {
      l4i->main_memory.low   = 0;
      l4i->main_memory.high  = (mb_info.mem_upper + 1024) << 10;
      l4i->dedicated[0].low  = mb_info.mem_lower          << 10;
      l4i->dedicated[0].high = 1                          << 20;
    }
  else
    {
      l4i->dedicated[1].low  = mod_range_start;
      l4i->dedicated[1].high = mod_range_end;
    }

  /* set up sigma0 info */
  if (sigma0)
    {
      l4i->sigma0_eip = sigma0_start;
      l4i->sigma0_memory.low = sigma0_low;
      l4i->sigma0_memory.high = sigma0_high;

      /* XXX UGLY HACK: Jochen's kernel can't pass args on a sigma0
         stack not within the L4 kernel itself -- therefore we use the
         kernel info page itself as the stack for sigma0.  the field
         we use is the task descriptor for the unused "ktest2" task */
      switch (l4_version)
	{
	case VERSION_L4_V2:
	case VERSION_L4_IBM:
	  l4i->sigma0_esp = 0x1060;
	  break;
	case VERSION_FIASCO:
	  l4i->sigma0_esp = sigma0_stack;
	  break;
	}
      printf("  Sigma0 config    eip:%08x esp:%08x [%08x-%08x]\n",
	     l4i->sigma0_eip, l4i->sigma0_esp,
	     l4i->sigma0_memory.low, l4i->sigma0_memory.high);
    }

  /* set up roottask info */
  if (roottask)
    {
      l4i->root_eip = roottask_start;
      l4i->root_memory.low = roottask_low;
      l4i->root_memory.high = roottask_high;
      printf("  Roottask config  eip:%08x esp:%08x [%08x-%08x]\n",
	     l4i->root_eip, l4i->root_esp,
	     l4i->root_memory.low, l4i->root_memory.high);
    }
}
#endif

/*****************************************************************************/
/*
 * load module
 */
/*****************************************************************************/
static l4_addr_t
load_module(unsigned module, l4_addr_t *begin, l4_addr_t *end)
{
  exec_task_t exec_task;
  exec_info_t exec_info;
  int r;

  exec_task.begin = 0xffffffff;
  exec_task.end = 0;

  exec_task.mod_start = (void *)(mb_mod[module].mod_start);
  exec_task.task_no = module;
  exec_task.mod_no = module;

  r = exec_load_elf(l4_exec_read, l4_exec_read_exec, &exec_task, &exec_info);

  if (r)
    printf("%s: exec_load_elf return value %d\n", __func__, r);

  *begin = exec_task.begin;
  *end = exec_task.end;

  /* Only add region if non-elf module */
  if (r)
    region_add(exec_task.begin, exec_task.end, (char*)mb_mod[module].cmdline);

  return (l4_addr_t) exec_info.entry;
}

/*****************************************************************************/
/*
 * copy mbi to save place for ourself
 */
/*****************************************************************************/
static void
copy_mbi_for_ourself(l4util_mb_info_t *dst_mbi, l4util_mb_info_t *src_mbi)
{
  int i;
  char *mod_names;

  /* copy (extended) multiboot info structure */
  memcpy(dst_mbi, src_mbi, sizeof(l4util_mb_info_t));

  /* copy command line, if available */
  if (dst_mbi->flags & L4UTIL_MB_CMDLINE)
    {
      strncpy(mb_cmdline, (char *) (dst_mbi->cmdline), sizeof(mb_cmdline));
      mb_cmdline[sizeof(mb_cmdline) - 1] = 0;
      dst_mbi->cmdline = (l4_addr_t) mb_cmdline;
    }

  /* copy extended VIDEO information, if available */
  if (dst_mbi->flags & L4UTIL_MB_VIDEO_INFO)
    {
      if (src_mbi->vbe_mode_info)
	{
	  memcpy(&mb_vbe_mode, (void*)(src_mbi->vbe_mode_info),
	      sizeof(l4util_mb_vbe_mode_t));
	  dst_mbi->vbe_mode_info = (l4_addr_t) &mb_vbe_mode;
	}

      /* copy VBE controller info structure */
      if (src_mbi->vbe_ctrl_info)
	{
	  memcpy(&mb_vbe_ctrl, (void*)(src_mbi->vbe_ctrl_info),
	      sizeof(l4util_mb_vbe_ctrl_t));
	  dst_mbi->vbe_ctrl_info = (l4_addr_t) &mb_vbe_ctrl;
	}
    }

  /* copy module descriptions */
  memcpy(mb_mod, (void *)dst_mbi->mods_addr,
	 dst_mbi->mods_count * sizeof (l4util_mb_mod_t));
  dst_mbi->mods_addr = (l4_addr_t) mb_mod;

  /* copy command lines of modules */
  mod_names = mb_mod_names;
  for (i = 0; i < dst_mbi->mods_count; i++)
    {
      char *name = (char *) mb_mod[i].cmdline;
      char *newstring = name ? name : "[unknown]";
      int len = strlen(newstring) + 1;

      if (mod_names - mb_mod_names + len > sizeof(mb_mod_names))
        {
	  printf("Bootstrap: mbi-cmdline overflow, aborting processing...\n");
	  break;
	}

      strcpy(mod_names, newstring); /* We already made sure the lengths are ok */
      if (name)
	mb_mod[i].cmdline = (l4_addr_t)mod_names;

      mod_names += len;
    }
}


/*****************************************************************************/
/*
 * copy mbi from bootstrap to roottask
 * also add the l4_version variable to command-line
 * XXX don't copy kernel, sigma0 and roottask module XXX
 */
/*****************************************************************************/
static void
copy_mbi_for_roottask(l4_umword_t imagecopy_addr)
{
  int i;

  /* XXX hacky, use first 4 pages to save mbi for roottask XXX */
  mb_info_addr      = (l4util_mb_info_t*)(roottask_start - 0x4000);
  mb_cmdline_addr   = (char *)(((unsigned long)(mb_info_addr + 1) + 3) & ~3);
  mb_vbe_mode_addr  = (l4util_mb_vbe_mode_t*)(((unsigned long)mb_cmdline_addr + sizeof(mb_cmdline) + 3) & ~3);
  mb_vbe_ctrl_addr  = (l4util_mb_vbe_ctrl_t*)(((unsigned long)(mb_vbe_mode_addr + 1) + 3) & ~3);
  mb_mod_addr       = (l4util_mb_mod_t*)(((unsigned long)(mb_vbe_ctrl_addr + 1) + 3) & ~3);
  mb_mod_names_addr = (char *)(((unsigned long)(mb_mod_addr + mb_info.mods_count-3) + 3) & ~3);

  /* copy multiboot info structure */
  memcpy(mb_info_addr, &mb_info, sizeof(l4util_mb_info_t));

  /* copy roottask command line, if available */
  if ((((l4util_mb_mod_t*)mb_info.mods_addr)[roottask_module]).cmdline)
    {
      /* Make sure to place l4_version as the first argument to not
       * disturb the configuration parser in the roottask */
      char *cl = (char*)((((l4util_mb_mod_t*)
                           mb_info.mods_addr)[roottask_module]).cmdline);
      char *p, oldval;
      if (!(p = strchr(cl, ' ')))
	p = cl + strlen(cl);
      oldval = *p;
      *p = 0;

      strncpy(mb_cmdline_addr, cl, sizeof(mb_cmdline));
      strncat(mb_cmdline_addr, " l4_version=", sizeof(mb_cmdline));
      snprintf(mb_cmdline_addr, sizeof(mb_cmdline), "%s%d",
	       mb_cmdline_addr, l4_version);
      strncat(mb_cmdline_addr, " ", sizeof(mb_cmdline));

      *p = oldval;
      strncat(mb_cmdline_addr, p, sizeof(mb_cmdline));

      mb_cmdline_addr[sizeof(mb_cmdline) - 1] = 0;
      mb_info_addr->cmdline = (l4_addr_t)mb_cmdline_addr;
    }

  /* copy extended VIDEO information, if available */
  if (mb_info_addr->flags & L4UTIL_MB_VIDEO_INFO)
    {
      if (mb_info.vbe_mode_info)
	{
	  memcpy(mb_vbe_mode_addr, (void*)(mb_info.vbe_mode_info),
	      sizeof(l4util_mb_vbe_mode_t));
	  mb_info_addr->vbe_mode_info = (l4_addr_t) mb_vbe_mode_addr;
	}

      /* copy VBE controller info structure */
      if (mb_info.vbe_ctrl_info)
	{
	  memcpy(mb_vbe_ctrl_addr, (void*)(mb_info.vbe_ctrl_info),
	      sizeof(l4util_mb_vbe_ctrl_t));
	  mb_info_addr->vbe_ctrl_info = (l4_addr_t) mb_vbe_ctrl_addr;
	}
    }

  /* Tell roottask where to find its unmodified image copy */
  mb_info_addr->syms.e.addr  = imagecopy_addr;

  /* copy module description */
  for (i = 0; i < mb_info.mods_count - 3; i++)
    {
      memcpy(&mb_mod_addr[i],
	     &((l4util_mb_mod_t *)mb_info.mods_addr)[i + 3],
	     sizeof(l4util_mb_mod_t));
    }
  mb_info_addr->mods_addr = (l4_addr_t) mb_mod_addr;
  mb_info_addr->mods_count = mb_info.mods_count - 3;

  /* copy command lines of modules */
  for (i = 0; i < mb_info.mods_count - 3; i++)
    {
      char * name = (char *) mb_mod[i + 3].cmdline;

      strncpy(mb_mod_names_addr, name ? name : "[unknown]", MOD_NAME_MAX);
      mb_mod_names_addr[MOD_NAME_MAX-1] = '\0'; // terminate string
      if (name)
        mb_mod_addr[i].cmdline = (l4_addr_t)mb_mod_names_addr;

      mb_mod_names_addr += strlen(mb_mod_names_addr) + 1;
    }
}

static void roottask_save_symbols_lines(l4_umword_t *e_addr)
{
  /* To transfer the symbol and lines info to the roottask with the minimal
   * effort in bootstrap, we just copy the whole roottask image to a new
   * location a just set the syms.e.addr field in the mbi. Roottask can then
   * figure out the rest itself, load the symbols/lines info and release
   * the memory. */

  int size = mb_mod[roottask_module].mod_end - mb_mod[roottask_module].mod_start;
  unsigned long start = l4_round_page(mod_range_end);

  region_add(start, start + size, ".Roottask syms/lines (copy)");

  memcpy((void *)start, (void *)mb_mod[roottask_module].mod_start, size);

  *e_addr  = start;
}


#ifdef IMAGE_MODE

#ifdef COMPRESS
#include "uncompress.h"
#endif

typedef struct
{
  void        *start;
  l4_umword_t size;
  l4_umword_t size_uncompressed;
  char const  *name;
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

#ifdef ARCH_x86
  /* some first cmdline scanning to get serial output working */
  /* XXX: merge with the other scanning */
  if (mbi->flags & L4UTIL_MB_CMDLINE)
    {
      int comport = 1;
      char *s;

      if ((s = strstr((char *) mbi->cmdline, " -comport=")))
	comport = strtoul(s+10,0,0);

      if (strstr((char *) mbi->cmdline, " -serial"))
	com_cons_init(comport);
    }
#endif

  mbi->mods_count = _module_info_end - _module_info_start;
  mbi->flags      |= L4UTIL_MB_MODS;
  mbi->mods_addr  = (l4_uint32_t)mods;

  assert(mbi->mods_count > 2);

  for (i = 0; i < mbi->mods_count; i++)
    {
#ifdef COMPRESS
      void *image = decompress(_module_info_start[i].name,
                               _module_info_start[i].start,
                               _module_info_start[i].size,
                               _module_info_start[i].size_uncompressed);
#else
      void *image = _module_info_start[i].start;
#endif
      mods[i].mod_start = (l4_uint32_t)image;
      mods[i].mod_end   = (l4_umword_t)image + _module_info_start[i].size_uncompressed - 1;
      printf("mod%02u: %08x - %08x: %s\n",
	     i, mods[i].mod_start, mods[i].mod_end, _module_info_start[i].name);
    }
}
#endif /* IMAGE_MODE */

#ifdef REALMODE_LOADING
unsigned long _mod_end;

static void
move_modules_to_modaddr(void)
{
  int i, count = _module_info_end - _module_info_start;
  unsigned char *a = (char *)_mod_addr;

  /* patch module_info structure with new start values */
  for (i = 0; i < count; i++)
    {
      memcpy(a, _module_info_start[i].start, _module_info_start[i].size);
      _module_info_start[i].start = a;
      a += l4_round_page(_module_info_start[i].size);
    }
  _mod_end = l4_round_page((unsigned long)a);
}
#else
extern char _module_data_start;
extern char _module_data_end;
const unsigned long _mod_start = (unsigned long)&_module_data_start;
const unsigned long _mod_end   = (unsigned long)&_module_data_end;
#endif /* REALMODE_LOADING */

/*****************************************************************************/
/**
 * \brief  Startup, started from crt0.S
 */
/*****************************************************************************/
void
startup(l4util_mb_info_t *mbi, unsigned int flag, void *realmode_or_xen_si_pointer)
{
  l4_kernel_info_t * l4i;
  l4util_mb_info_t l4_mbi;

#ifdef ARCH_x86
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
#else
  /* arm */
  mbi = init_loader_mbi(NULL, MEMORY << 10);
#endif

#ifdef IMAGE_MODE
  construct_mbi(mbi);
#endif


  assert(mbi->flags & L4UTIL_MB_MODS); /* we need at least two boot modules: */
  assert(mbi->flags & L4UTIL_MB_MEMORY);
  assert(mbi->mods_count >= 2);	/* 1 = L4 kernel, 2 = first user task */
  assert(mbi->mods_count <= MODS_MAX);

  /* parse command line */
  if (mbi->flags & L4UTIL_MB_CMDLINE)
    {
#ifdef ARCH_x86
      int comport = 1;
      char *s;

      if ((s = strstr((char *) mbi->cmdline, " -comport=")))
	comport = strtoul(s+10,0,0);

      if (strstr((char *) mbi->cmdline, " -serial"))
	com_cons_init(comport);

      if (strstr((char *) mbi->cmdline, " -hercules") && have_hercules())
        hercules = 1;
#endif

      if (strstr((char *) mbi->cmdline, " -sigma0"))
	sigma0 = 1;

      if (strstr((char *) mbi->cmdline, " -roottask"))
	roottask = 1;
    }

  /* we're not an L4 task yet -- we're just a GRUB-booted kernel! */

  puts("\nL4 Bootstrapper");

  /* copy Multiboot data structures, we still need to a safe place
   * before playing with memory we don't own and starting L4
   */
  copy_mbi_for_ourself(&mb_info, mbi);

  /* shouldn't touch original Multiboot parameters after here */

  /* Guess the amount of installed memory from the bootloader. We already
   * need that value because:
   * 1. we have to initialize the memmap for module loading.
   * 2. we have to check for a valid L4 kernel.
   *
   * Later (in init.c), mem_high is determined from the sigma0 server.
   */
  setup_memory_range(&mb_info);

  /* Calulate the start address and end adress of loaded modules. We need
   * that value because:
   * we have to initialize the memmap for module loading
   */

  /* We initialize the memmap here to observe loading tasks before starting
   * the L4 kernel. Later in init.c, we re-initialize the memmap
   **/
  init_memmap(&mb_info);

  /* We initialize the region map here to find overlapping regions and
   * report useful error messages
   * roottask will use its own region map
   **/
  init_region(&mb_info);

  /* setup kernel PART ONE */
  if (kernel)
    {
      printf("  Loading ");
      print_module_name((const char*)mb_mod[0].cmdline, "[KERNEL]");
      putchar('\n');

      kernel_start = load_module(kernel_module, &kernel_low, &kernel_high);

      /* check for L4 version */
      l4_version = check_l4_version((unsigned char *)kernel_low,
				    (unsigned char *)kernel_high,
				    &abi_version);
    }
  /* setup sigma0 */
  if (sigma0)
    {
      printf("  Loading ");
      print_module_name((const char*)mb_mod[sigma0_module].cmdline, 
			 "[SIGMA0]");
      putchar('\n');

      sigma0_start = load_module(sigma0_module, &sigma0_low, &sigma0_high);
      sigma0_stack =
	(l4_addr_t)(sigma0_init_stack + sizeof(sigma0_init_stack));
    }

  /* setup roottask */
  if (roottask)
    {
      l4_umword_t imagecopy_addr;

      printf("  Loading ");
      print_module_name((const char*)mb_mod[roottask_module].cmdline,
			 "[ROOTTASK]");
      putchar('\n');

      /* Save symbol/lines information */
      roottask_save_symbols_lines(&imagecopy_addr);

      roottask_start = load_module(roottask_module,
				   &roottask_low, &roottask_high);
      roottask_stack = (l4_addr_t)roottask_init_stack +
				   sizeof(roottask_init_stack);
      copy_mbi_for_roottask(imagecopy_addr);
    }

  /* setup kernel PART TWO */
  /* do some special kernel initialization */
  switch (l4_version)
    {
#ifdef ARCH_x86
    case VERSION_L4_V2:
      /* init old GMD-Version */
      l4i = (l4_kernel_info_t *) kernel_start;
      {
        exec_info_t exec_info;
        init_l4_gmd(l4i, &exec_info);
      }
      break;

    case VERSION_L4_IBM:
      l4i = (l4_kernel_info_t *) kernel_start;
      init_ibm_nucleus(l4i);
      break;
#endif

    case VERSION_FIASCO:
      init_fiasco(&l4i);
      break;

#ifdef ARCH_x86
    case VERSION_L4_KA:
      init_hazelnut(&l4i);
      break;
#endif

    default:
      l4i = 0;
      printf("  Could not identify version of the kernel\n");
    }

  /* setup multi boot info structure for kernel */
  l4_mbi = mb_info;
  l4_mbi.flags = L4UTIL_MB_MEMORY;
  if (mb_mod[0].cmdline)
    {
      l4_mbi.cmdline = mb_mod[0].cmdline;
      l4_mbi.flags |= L4UTIL_MB_CMDLINE;
    }

#ifdef ARCH_x86
  /* setup the L4 kernel info page before booting the L4 microkernel:
     patch ourselves into the booter task addresses */
  init_kip(l4i);

  if (abi_version == VERSION_ABI_V4)
    {
      printf ("  Initializing v4 KIP\n");
      init_v4_kip (l4i);
    }
#endif

  regions_dump();

  printf("  Starting kernel ");
  print_module_name((const char*)mb_mod[0].cmdline, "[KERNEL]");
  printf(" at 0x%08x\n", kernel_start);

#ifdef ARCH_x86
  asm volatile
    ("pushl $exit\n\t"
     "pushl %3  \n\t"
     "ret\n\t"
     :				/* no output */
     : "a" (L4UTIL_MB_VALID),
       "b" (&l4_mbi),
       "S" (realmode_or_xen_si_pointer),
       "r" (kernel_start));
#endif

#ifdef ARCH_arm
  {
    typedef void (*startup_func)(void);
    startup_func f = (startup_func)kernel_start;
    unsigned long p = (unsigned long)kernel_start;
    int i;

    p &= 0xfffff000; /* 4k align */

    /* Search for the KIP in the kernel image */
    l4_kernel_info_t *kip = NULL;

    for (i = 1; i <= 5; i++)
      {
	l4_kernel_info_t *k = (l4_kernel_info_t *)(p + i * 0x1000);
	if (k->magic == L4_KERNEL_INFO_MAGIC)
	  {
	    kip = k;
	    break;
	  }
      }

    if (!kip)
      panic("ERROR: No kernel info page found in kernel binary image!");

    printf("KIP is at %p\n", kip);

    kip->sigma0_eip = sigma0_start;
    kip->root_eip   = roottask_start;

    l4_kernel_info_mem_desc_t *md = l4_kernel_info_get_mem_descs(kip);
    l4_kernel_info_set_mem_desc(md, RAM_BASE, RAM_BASE + ((MEMORY << 20) - 1),
				l4_mem_type_conventional, 0, 0);
    l4_kernel_info_set_mem_desc(++md, kernel_low, kernel_high - 1,
				l4_mem_type_reserved, 0, 0);
    l4_kernel_info_set_mem_desc(++md, sigma0_low, sigma0_high - 1,
				l4_mem_type_reserved, 0, 0);
    l4_kernel_info_set_mem_desc(++md, roottask_low, roottask_high - 1,
				l4_mem_type_reserved, 0, 0);

    if (_module_data_start < _module_data_end)
      {
	l4_kernel_info_set_mem_desc(++md, (l4_umword_t)_module_data_start,
				    (l4_umword_t)_module_data_end - 1,
				    l4_mem_type_bootloader, 0, 0);
      }

    printf("Starting kernel... (%x)\n", 0);
    f();
    panic("Returned from kernel?!");
  }
#endif

  /*NORETURN*/

}

static int
l4_exec_read(void *handle, l4_addr_t file_ofs,
		     void *buf, l4_size_t size,
		     l4_size_t *out_actual)
{
  struct exec_task *e = (struct exec_task *) handle;
  memcpy(buf, (char*)(e->mod_start) + file_ofs, size);

  *out_actual = size;
  return 0;
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

  region_add(mem_addr, mem_addr + mem_size,
             mb_mod[exec_task->mod_no].cmdline
	       ? (const char*)mb_mod[exec_task->mod_no].cmdline
	       :  ".[Unknown]");

  memcpy((void *) mem_addr,
         (char*)(exec_task->mod_start) + file_ofs, file_size);
  if (file_size < mem_size)
    memset((void *) (mem_addr + file_size), 0, mem_size - file_size);

  return 0;
}
