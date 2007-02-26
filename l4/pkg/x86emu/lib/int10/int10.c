/* $Id$ */
/**
 * \file	x86emu/lib/int10/int10.c
 * \brief	Call VESA BIOS functions using the real mode interface
 *
 * \date	2005
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/rmgr/librmgr.h>
#include <l4/l4rm/l4rm.h>
#include <l4/x86emu/x86emu.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/mb_info.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <l4/x86emu/int10.h>

#undef DBG_IO
#undef DBG_MEM

static u8   my_inb(X86EMU_pioAddr addr) X86API;
static u16  my_inw(X86EMU_pioAddr addr) X86API;
static u32  my_inl(X86EMU_pioAddr addr) X86API;
static void my_outb(X86EMU_pioAddr addr, u8 val) X86API;
static void my_outw(X86EMU_pioAddr addr, u16 val) X86API;
static void my_outl(X86EMU_pioAddr addr, u32 val) X86API;
static u8   my_rdb(u32 addr) X86API;
static u16  my_rdw(u32 addr) X86API;
static u32  my_rdl(u32 addr) X86API;
static void my_wrb(u32 addr, u8 val) X86API;
static void my_wrw(u32 addr, u16 val) X86API;
static void my_wrl(u32 addr, u32 val) X86API;

static l4_addr_t   v_page[1024*1024/(L4_PAGESIZE)];
static l4_uint32_t v_area;
static l4_umword_t initialized;

static void
warn(u32 addr, const char *func)
{
  printf("\033[31mWARNING: Function %s access %08lx\033[m\n", func, addr);
}

static u8 X86API
my_inb(X86EMU_pioAddr addr)
{
  int r;
  asm volatile ("inb %w1, %b0" : "=a" (r) : "d" (addr));
#ifdef DBG_IO
  printf("%04x:%04x inb %x -> %x\n", M.x86.R_CS, M.x86.R_IP, addr, r);
  l4_sleep(10);
#endif
  return r;
}

static u16 X86API
my_inw(X86EMU_pioAddr addr)
{
  u16 r;
  asm volatile ("inw %w1, %w0" : "=a" (r) : "d" (addr));
#ifdef DBG_IO
  printf("%04x:%04x inw %x -> %x\n", M.x86.R_CS, M.x86.R_IP, addr, r);
  l4_sleep(10);
#endif
  return r;
}

static u32 X86API
my_inl(X86EMU_pioAddr addr)
{
  u32 r;
  asm volatile ("inl %w1, %0" : "=a" (r) : "d" (addr));
#ifdef DBG_IO
  printf("%04x:%04x inl %x -> %lx\n", M.x86.R_CS, M.x86.R_IP, addr, r);
  l4_sleep(10);
#endif
  return r;
}

static void X86API
my_outb(X86EMU_pioAddr addr, u8 val)
{
#ifdef DBG_IO
  printf("%04x:%04x outb %x -> %x\n", M.x86.R_CS, M.x86.R_IP, val, addr);
  l4_sleep(10);
#endif
  asm volatile ("outb %b0, %w1" : "=a" (val), "=d" (addr)
                                : "a" (val), "d" (addr));
}

static void X86API
my_outw(X86EMU_pioAddr addr, u16 val)
{
#ifdef DBG_IO
  printf("%04x:%04x outw %x -> %x\n", M.x86.R_CS, M.x86.R_IP, val, addr);
  l4_sleep(10);
#endif
  asm volatile ("outw %w0, %w1" : "=a" (val), "=d" (addr)
                                : "a" (val), "d" (addr));
}

static void X86API
my_outl(X86EMU_pioAddr addr, u32 val)
{
#ifdef DBG_IO
  printf("%04x:%04x outl %lx -> %x\n", M.x86.R_CS, M.x86.R_IP, val, addr);
  l4_sleep(10);
#endif
  asm volatile ("outl %0, %w1" : "=a"(val), "=d" (addr) 
                               : "a" (val), "d" (addr));
}

static u8 X86API
my_rdb(u32 addr)
{
  if (addr > 1 << 20)
    warn(addr, __FUNCTION__);

#ifdef DBG_MEM
  printf("readb %08lx->%08lx => %02x\n",
      addr, v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE), 
      *(u8*)(v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE)));
  l4_sleep(10);
#endif
  return *(u32*)(v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE));
}

static u16 X86API
my_rdw(u32 addr)
{
  if (addr > 1 << 20)
    warn(addr, __FUNCTION__);

#ifdef DBG_MEM
  printf("readw %08lx->%08lx => %04x\n",
      addr, v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE),
      *(u16*)(v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE)));
  l4_sleep(10);
#endif
  return *(u16*)(v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE));
}

static u32 X86API
my_rdl(u32 addr)
{
  if (addr > 1 << 20)
    warn(addr, __FUNCTION__);

#ifdef DBG_MEM
  printf("readl %08lx->%08lx => %08lx\n",
      addr, v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE), 
      *(u32*)(v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE)));
  l4_sleep(10);
#endif
  return *(u32*)(v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE));
}

static void  X86API
my_wrb(u32 addr, u8 val)
{
  if (addr > 1 << 20)
    warn(addr, __FUNCTION__);

#ifdef DBG_MEM
  printf("writeb %08lx->%08lx => %02x\n",
      addr, v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE), val);
  l4_sleep(10);
#endif
  *(u8*)(v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE)) = val;
}

static void X86API
my_wrw(u32 addr, u16 val)
{
  if (addr > 1 << 20)
    warn(addr, __FUNCTION__);

#ifdef DBG_MEM
  printf("writew %08lx->%08lx => %04x\n",
      addr, v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE), val);
  l4_sleep(10);
#endif
  *(u16*)(v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE)) = val;
}

static void X86API
my_wrl(u32 addr, u32 val)
{
  if (addr > 1 << 20)
    warn(addr, __FUNCTION__);

#ifdef DBG_MEM
  printf("writel %08lx->%08lx => %08lx\n",
      addr, v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE), val);
  l4_sleep(10);
#endif
  *(u32*)(v_page[addr/L4_PAGESIZE] + (addr % L4_PAGESIZE)) = val;
}

X86EMU_pioFuncs my_pioFuncs =
{
  my_inb,  my_inw,  my_inl,
  my_outb, my_outw, my_outl
};

X86EMU_memFuncs my_memFuncs =
{
  my_rdb, my_rdw, my_rdl,
  my_wrb, my_wrw, my_wrl
};

void
printk(const char *format, ...)
{
  va_list list;
  va_start(list, format);
  vprintf(format, list);
  va_end(list);
}

static int
x86emu_int10_init(void)
{
  int error;
  l4_addr_t addr;
  l4_uint32_t idx;
  l4_umword_t dummy;
  l4_msgdope_t result;
  l4dm_dataspace_t ds;

  if (initialized)
    return 0;

  M.x86.debug = 0 /*| DEBUG_DECODE_F*/;
  
  X86EMU_setupPioFuncs(&my_pioFuncs);
  X86EMU_setupMemFuncs(&my_memFuncs);

  /* Reserve region for physical memory 0x00000...0xfffff. Make sure that we
   * can unmap it using one single l4_fpage_unmap (alignment). */
  if ((error = l4rm_area_reserve(1<<20, L4RM_LOG2_ALIGNED,
				 &v_page[0], &v_area)))
    {
      printf("Error %d reserving area for x86emu\n", error);
      return error;
    }

  /* Map physical page 0x00000 */
  if ((error = rmgr_get_page0((void*)v_page[0])))
    {
      printf("Error %d allocating physical page 0 for x86emu\n", error);
      return error;
    }

  if ((error = l4dm_mem_open(L4DM_DEFAULT_DSM, L4_PAGESIZE, 0, 0,
			     "int10 code", &ds)))
    {
      printf("Error %d allocating page for x86emu\n", error);
      return error;
    }

  /* Map dummy as physical page 0x01000 */
  v_page[1] = v_page[0] + L4_PAGESIZE;
  if ((error = l4rm_area_attach_to_region(&ds, v_area, (void*)v_page[1],
					  L4_PAGESIZE, 0, L4DM_RW | L4RM_MAP)))
    {
      printf("Error %d attaching page for x86emu\n", error);
      return error;
    }

  /* Map 0x9F000...0x100000 */
  for (idx=0x09F000/L4_PAGESIZE, addr=0x09F000;
                                 addr<0x100000;
       idx++,			 addr+=L4_PAGESIZE)
    {
      v_page[idx] = v_page[0] + addr;
      error = l4_ipc_call(rmgr_pager_id,
			  L4_IPC_SHORT_MSG, addr, 0,
			  L4_IPC_MAPMSG(v_page[idx], L4_LOG2_PAGESIZE),
			  &dummy, &dummy, L4_IPC_NEVER, &result);
      if (error || !l4_ipc_fpage_received(result))
	Panic("Cannot map page at %08lx from rmgr", addr);
    }

  /* int 10 ; ret */
  *(l4_uint8_t*)(v_page[1]+0) = 0xcd;
  *(l4_uint8_t*)(v_page[1]+1) = 0x10;
  *(l4_uint8_t*)(v_page[1]+2) = 0xf4;

  initialized = 1;

  return 0;
}

int
x86emu_int10_done(void)
{
  int ret;

  if (!initialized)
    return 0;

  /* Unmap physical memory */
  l4_fpage_unmap(l4_fpage(v_page[0], 20, 0, 0),
		 L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

  /* Free the area */
  ret = l4rm_area_release(v_area);

  initialized = 0;

  return ret;
}

int
x86emu_int10_set_vbemode(int mode, l4util_mb_vbe_ctrl_t **ctrl_info,
			 l4util_mb_vbe_mode_t **mode_info)
{
  int error;
  l4_addr_t phys;
  char *oem_string;

  if ((error = x86emu_int10_init()))
    return error;

  printf("Trying execution of ``set VBE mode'' using x86emu\n");

  /* Get VESA BIOS controller information. */
  M.x86.R_EAX  = 0x4F00;	/* int10 function number */
  M.x86.R_EDI  = 0x100;		/* ES:DI pointer to at least 512 bytes */
  M.x86.R_IP   = 0;		/* address of "int10; hlt" */
  M.x86.R_SP   = L4_PAGESIZE;	/* SS:SP pointer to stack */
  M.x86.R_CS   =
  M.x86.R_DS   =
  M.x86.R_ES   =
  M.x86.R_SS   = L4_PAGESIZE >> 4;
  X86EMU_exec();
  *ctrl_info = (l4util_mb_vbe_ctrl_t*)(v_page[1] + 0x100);
  if (M.x86.R_AX != 0x4F)
    {
      printf("VBE BIOS not present.\n");
      return -L4_EINVAL;
    }

  phys =  ((*ctrl_info)->oem_string        & 0x0FFFF)
       + (((*ctrl_info)->oem_string >> 12) & 0xFFFF0);
  oem_string = (char*)(v_page[phys/L4_PAGESIZE] + (phys % L4_PAGESIZE));
  printf("Found VESA BIOS version %d.%d\n"
         "OEM %s\n",
      (int)((*ctrl_info)->version >> 8),
      (int)((*ctrl_info)->version &  0xFF),
      (*ctrl_info)->oem_string ? oem_string : "[unknown]");
  if ((*ctrl_info)->version < 0x0200)
    {
      printf("VESA BIOS 2.0 or later required.\n");
      return -L4_EINVAL;
    }

  /* Get mode information. */
  M.x86.R_EAX  = 0x4F01;	/* int10 function number */
  M.x86.R_ECX  = mode;		/* VESA mode */
  M.x86.R_EDI  = 0x800;		/* ES:DI pointer to at least 256 bytes */
  M.x86.R_IP   = 0;		/* address of "int10; hlt" */
  M.x86.R_SP   = L4_PAGESIZE;	/* SS:SP pointer to stack */
  M.x86.R_CS   =
  M.x86.R_DS   =
  M.x86.R_ES   =
  M.x86.R_SS   = L4_PAGESIZE >> 4;
  X86EMU_exec();

  *mode_info = (l4util_mb_vbe_mode_t*)(v_page[1] + 0x800);
  if (M.x86.R_AX != 0x004F ||
      ((*mode_info)->mode_attributes & 0x0091) != 0x0091)
    {
      printf("Mode %x not supported\n", mode);
      return -L4_EINVAL;
    }

  /* Switch mode. */
  M.x86.R_EAX  = 0x4F02;	/* int10 function number */
  M.x86.R_EBX  = mode & 0xf7ff;	/* VESA mode; use current refresh rate */
  M.x86.R_EBX |= (1<<14);	/* use flat buffer model */
  M.x86.R_IP   = 0;		/* address of "int10; hlt" */
  M.x86.R_SP   = L4_PAGESIZE;	/* SS:SP pointer to stack */
  M.x86.R_CS   =
  M.x86.R_DS   =
  M.x86.R_ES   =
  M.x86.R_SS   = L4_PAGESIZE >> 4;
  X86EMU_exec();

  printf("VBE mode 0x%x successfully set.\n", mode);
  return 0;
}

int
x86emu_int10_pan(unsigned *x, unsigned *y)
{
  int error;

  if ((error = x86emu_int10_init()))
    return error;
  
  printf("Trying execution of ``set display start'' using x86emu\n");

  /* Get VESA BIOS controller information. */
  M.x86.R_EAX  = 0x4F07;	/* int10 function number */
  M.x86.R_BL   = 0x00;          /* set display start */
  M.x86.R_CX   = *x;            /* first displayed pixel in scanline */
  M.x86.R_DX   = *y;            /* first displayed scanline */
  M.x86.R_IP   = 0;		/* address of "int10; hlt" */
  M.x86.R_SP   = L4_PAGESIZE;	/* SS:SP pointer to stack */
  M.x86.R_CS   =
  M.x86.R_DS   =
  M.x86.R_ES   =
  M.x86.R_SS   = L4_PAGESIZE >> 4;
  X86EMU_exec();

  if (M.x86.R_AX != 0x004F)
    {
      printf("Panning to %d,%d not supported\n", *x, *y);
      return -L4_EINVAL;
    }

  printf("Successful.\n");
  return 0;
}
