/*!
 * \file	matrox_regs.h
 * \brief	Matrox Gxx driver
 *
 * \date	07/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __MATROX_REGS_H_
#define __MATROX_REGS_H_

extern l4_addr_t mga_mmio_vbase;


static inline unsigned 
mga_readb(unsigned va, unsigned offs) 
{
  return *(volatile unsigned char*)(va + offs);
}

static inline unsigned 
mga_readw(unsigned va, unsigned offs) 
{
  return *(volatile unsigned short*)(va + offs);
}

static inline unsigned 
mga_readl(unsigned va, unsigned offs) 
{
  return *(volatile unsigned*)(va + offs);
}

static inline void 
mga_writeb(unsigned va, unsigned offs, unsigned char value) 
{
  *(volatile unsigned char*)(va + offs) = value;
}

static inline void 
mga_writew(unsigned va, unsigned offs, unsigned short value) 
{
  *(volatile unsigned short*)(va + offs) = value;
}

static inline void 
mga_writel(unsigned va, unsigned offs, unsigned value) 
{
  *(volatile unsigned *)(va + offs) = value;
}

#define mga_inb(addr)      mga_readb(mga_mmio_vbase, (addr))
#define mga_inl(addr)      mga_readl(mga_mmio_vbase, (addr))
#define mga_outb(addr,val) mga_writeb(mga_mmio_vbase, (addr), (val))
#define mga_outw(addr,val) mga_writew(mga_mmio_vbase, (addr), (val))
#define mga_outl(addr,val) mga_writel(mga_mmio_vbase, (addr), (val))

#define mga_readr(port,idx)     (mga_outb((port),(idx)), mga_inb((port)+1))
#define mga_setr(addr,port,val)  mga_outw(addr, ((val)<<8) | (port))

#define mga_fifo(n)	do {} while ((mga_inl(M_FIFOSTATUS) & 0xFF) < (n))
#define WaitTillIdle()	do {} while (mga_inl(M_STATUS) & 0x10000)

#endif

