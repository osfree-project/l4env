/*!
 * \file	loader/server/src/emulate.h
 * \brief	experimental
 *
 * \date	2003
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __EMULATE_H_
#define __EMULATE_H_

#include <l4/sys/types.h>
#include "app.h"

typedef union
{
  l4_umword_t raw;
  struct
    {
      unsigned  flag:1;		/* distinguish between adapter pf/emu */
      unsigned  write:1;	/* 0=read-only, 1=writeable */
      unsigned  desc:4;		/* descriptor number (0..15) */
      unsigned  size:2;		/* 0=1 byte,1=2 bytes, 2=4 bytes */
      unsigned  offs:22;	/* max size is 4MB */
      unsigned	set3:2;		/* set to 11b (>=0x40000000 == adapter page) */
    } emu;
} emu_addr_t;

struct emu_desc_s_t;
typedef struct emu_desc_s_t
{
  app_t		*app;
  l4_addr_t	map_addr;
  l4_addr_t	phys_addr;
  l4_size_t	size;
  const char	*name;
  void          (*init)  (struct emu_desc_s_t *desc);
  void		(*handle)(struct emu_desc_s_t *desc, 
			  emu_addr_t ea, l4_umword_t value, 
			  l4_umword_t *dw1, l4_umword_t *dw2, void **reply);
  void		(*spec)  (struct emu_desc_s_t *desc,
			  emu_addr_t ea, l4_umword_t value, 
			  l4_umword_t *dw1, l4_umword_t *dw2, void **reply);
  void		*private_mem;
  void		*private_mem_phys;
  l4_addr_t	private_virt_to_phys;
} emu_desc_t;

int  init_mmio_emu(void);
int  handle_mmio_emu(app_t *app, emu_addr_t ea, l4_umword_t value,
		     l4_umword_t *dw1, l4_umword_t *dw2, void **reply);
void check_mmio_emu (app_t *app, l4_umword_t pfa, 
		     l4_fpage_struct_t *fp, void *reply)
  __attribute__((regparm(3)));

extern inline l4_addr_t
desc_priv_virt_to_phys(emu_desc_t *desc, l4_addr_t addr);

extern inline l4_addr_t
desc_priv_phys_to_virt(emu_desc_t *desc, l4_addr_t addr);

extern inline l4_addr_t
desc_priv_virt_to_phys(emu_desc_t *desc, l4_addr_t addr)
{
  return addr + desc->private_virt_to_phys;
}

extern inline l4_addr_t
desc_priv_phys_to_virt(emu_desc_t *desc, l4_addr_t addr)
{
  return addr - desc->private_virt_to_phys;
}

#endif

