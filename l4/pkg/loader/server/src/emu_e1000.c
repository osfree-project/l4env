/* Some experimental code for emulating mmio of E1000 cards */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "emulate.h"
#include "pager.h"

#include <stdio.h>

#include <l4/crtx/ctor.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/ktrace.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/rmgr/librmgr.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/bitops.h>
#include <l4/util/irq.h>
#include <l4/util/port_io.h>

#define LIST_SIZE_RX   80
#define LIST_SIZE_TX  256

// Device rx/tx descriptor
typedef struct
{
  l4_uint64_t  buffer_addr;
  l4_uint16_t  length;
  l4_uint16_t  csum;
  volatile l4_uint8_t status;
  l4_uint8_t   errors;
  l4_uint16_t  special;
} e1000_desc_t;

// Descriptors are shared ro with driver.
typedef struct
{
  e1000_desc_t rx_desc[LIST_SIZE_RX];
  e1000_desc_t tx_desc[LIST_SIZE_TX];
} e1000_shared_area_t;

// Owner bitmaps are shared rw with driver.
typedef struct
{
  l4_uint8_t rx_bitmap[64];	// space for 512 descriptors
  l4_uint8_t tx_bitmap[64];	// space for 512 descriptors
} e1000_shared_own_t;

// Local data not shared with driver.
typedef struct
{
  e1000_desc_t *drv_list;	// address of descriptor ring
  e1000_desc_t *dev_list;	// address of descriptor ring (dev ring)
  l4_uint32_t   drv_entries;	// ring descriptor entries
  l4_uint32_t   dev_entries;	// ring descriptor entries (dev ring)
  l4_uint32_t   next;		// index of current descriptor
  l4_uint32_t   tail;		// index of last valid descriptor
  l4_uint8_t   *owner_map;
  l4_uint32_t   ctl;
} e1000_list_t;

static e1000_list_t rx, tx;

static inline void
e1000_outl(emu_desc_t *desc, unsigned value, unsigned offs)
{
  *(volatile l4_uint32_t*)(desc->map_addr+offs) = value;
}

static inline l4_uint32_t
e1000_inl(emu_desc_t *desc, unsigned offs)
{
  return *(volatile l4_uint32_t*)(desc->map_addr+offs);
}

static inline void
e1000_set_list(emu_desc_t *desc, e1000_desc_t *d, unsigned reg)
{
  l4_uint32_t addr = desc_priv_virt_to_phys(desc, (l4_addr_t)d);
  e1000_outl(desc, addr, reg);
}

#if 0
// check if memory range from addr until addr+size belongs to driver's
// address space
static void fastcall
e1000_check_mem_range(app_t *app, l4_addr_t addr, l4_size_t size)
{
  // only check if receive engine is active else the receise list
  // could still be uninitialized
  // only check if size is != 0 else address is not used
  if (size)
    {
      if (!addr_app_to_here(app, addr) || !addr_app_to_here(app, addr+size))
	{
	  app_msg(app, "Rx/Tx at %08x-%08x invalid", addr, addr+size);
	  enter_kdebug("FATAL");
	}
    }
}
#endif

static inline void
e1000_copy_drv_to_dev(app_t *app, e1000_desc_t *drv, e1000_desc_t *dev)
{
  dev->buffer_addr = drv->buffer_addr;
  dev->length      = drv->length;
  dev->csum        = drv->csum;
  dev->errors      = drv->errors;
  dev->special     = drv->special;
  asm volatile ("" : : : "memory");
  dev->status      = 0;
}

// Copy entry status from driver`s ring list to our`s and set device pointer.
static void fastcall
e1000_update_list(emu_desc_t *desc, e1000_list_t *l)
{
  int i, next;
  l4_uint8_t bit;
  l4_uint8_t *byte;

  if (!l->drv_list || !l->drv_entries || !(l->ctl & 2))
    return;

  next = l->next;
  bit  = 1 <<          (next % 8);
  byte = l->owner_map + next / 8;

  for (i=0; i<l->dev_entries; i++)
    {
      if (next == l->tail)
	{
	  l->next = next;
	  if (l == &tx)
	    {
	      next++;
	      if (next >= l->drv_entries)
		next = 0;
	      memset(&l->dev_list[next], 0, sizeof(l->dev_list[next]));
	    }
	  return;
	}

      if (!(*byte & bit))
	{
	  e1000_copy_drv_to_dev(desc->app, &l->drv_list[next],
					   &l->dev_list[next]);
	  *byte |= bit;
	}
//      else
//	printf("Skipping %s\n", l == &rx ? "RX" : "TX");
      next++;
      asm ("rolb %0 ; adc %4,%1" 
     	  : "=rm"(bit), "=rm"(byte) 
	  :   "0"(bit),   "1"(byte), "ir"(0));
      if (next >= l->drv_entries)
	{
	  next = 0;
	  bit  = 1;
	  byte = l->owner_map;
	}
    }

  app_msg(desc->app, "update: ring > %d! drv=%08x dev=%08x",
	  l->dev_entries, (unsigned)l->drv_list, (unsigned)l->dev_list);
  enter_kdebug("stop");
}

static void
e1000_handle(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
	     l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  int do_write = 1;

  if (ea.emu.write)
    {
      l4_umword_t offs = ea.emu.offs;
      switch (offs)
	{
	case 0x00100: /* RX Control - RW */
	  rx.ctl = value;
	  break;
	case 0x00400: /* TX Control - RW */
	  tx.ctl = value;
	  break;
	case 0x02800: /* RDBAL: RX Descriptor Base Address Low - RW */
	  rx.drv_list = (e1000_desc_t*)addr_app_to_here(desc->app, value);
	  app_msg(desc->app, "\033[32mSet rx list %08x\033[m",
	      (unsigned)rx.drv_list);
	  rx.next = 0;
	  e1000_update_list(desc, &rx);
	  e1000_set_list(desc, rx.dev_list, offs);
	  do_write = 0;
	  break;
	case 0x02808: /* RDLEN: Descriptor Length - RW */
	  app_msg(desc->app, "\033[32mSet rx size %d bytes", value);
	  rx.drv_entries = value / sizeof(e1000_desc_t);
	  break;
	case 0x02810: /* RDH:   RX Descriptor Head - RW */
	  app_msg(desc->app, "\033[32mHead rx %08x\033[m", value);
	  rx.next = value;
	  break;
	case 0x02818: /* RDT:   RX Descriptor Tail - RW */
	  rx.tail = value;
	  e1000_update_list(desc, &rx);
	  break;
	case 0x03800: /* TDBAL: TX Descriptor Base Address Low - RW */
	  tx.drv_list = (e1000_desc_t*)addr_app_to_here(desc->app, value);
	  app_msg(desc->app, "\033[32mSet tx list %08x\033[m",
	      (unsigned)tx.drv_list);
	  tx.next = 0;
	  e1000_update_list(desc, &tx);
	  e1000_set_list(desc, tx.dev_list, offs);
	  do_write = 0;
	  break;
	case 0x03808: /* TDLEN: TX Descriptor Length - RW */
	  app_msg(desc->app, "\033[32mSet tx size %d bytes", value);
	  tx.drv_entries = value / sizeof(e1000_desc_t);
	  break;
	case 0x03810: /* TDH:   TX Descriptor Head - RW */
	  app_msg(desc->app, "\033[32mHead tx %08x\033[m", value);
	  tx.next = value;
	  break;
	case 0x03818: /* TDT:   TX Descriptor Tail - RW */
	  tx.tail = value;
	  e1000_update_list(desc, &tx);
	  break;
	}

      if (do_write)
	{
	  /* handle write request */
	  switch (ea.emu.size)
	    {
	    case 0: // 1 byte
	    case 1: // 2 bytes
	      app_msg(desc->app, "E1000 size %d byte(s)?", 1 << ea.emu.size);
	      enter_kdebug("stop");
	      break;
	    case 2: // 4 bytes
	      e1000_outl(desc, value, offs);
	      break;
	    }
	}
    }
  else
    *dw1 = e1000_inl(desc, ea.emu.offs);

  *reply = L4_IPC_SHORT_MSG;
}

// special command
static void
e1000_spec(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
	   l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  if (ea.emu.size == 1)
    {
      // map shared area into client
      if (value == 0)
	{
	  *dw1 = 0;
	  *dw2 = l4_fpage(desc->private_mem1, desc->private_mem1_log2size,
			   L4_FPAGE_RO, L4_FPAGE_MAP).fpage;
	  *reply = L4_IPC_SHORT_FPAGE;
	}
      else if (value == 1)
	{
	  *dw1 = 0;
	  *dw2 = l4_fpage(desc->private_mem2, desc->private_mem2_log2size,
			   L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
	  *reply = L4_IPC_SHORT_FPAGE;
	}
      else if (value == 2)
	{
	  // 32 map IPCs since flexpages must be size-aligned
	  static int state;

	  if (state == 32)
	    {
	      *reply = L4_IPC_SHORT_MSG;
	      *dw1 = *dw2 = 0;
	      state = 0;
	    }
	  else
	    {
	      *reply = L4_IPC_SHORT_FPAGE;
	      *dw1 = state * 0x1000;
	      *dw2 = l4_fpage((l4_addr_t)desc->map_addr+*dw1, 12,
			      state == 2 || state == 3 
			        ? L4_FPAGE_RO : L4_FPAGE_RW,
			      L4_FPAGE_MAP).fpage;
	      state++;
	    }
	}
      else
	enter_kdebug("bad value");
    }
  else if (ea.emu.size == 2)
    {
      // driver changed list, copy changed information to dev_list
      // (called after driver interrupt handler)
      e1000_update_list(desc, &rx);
      e1000_update_list(desc, &tx);
      *reply = L4_IPC_SHORT_MSG;
    }
  else if (ea.emu.size == 3)
    {
      // return device settings to client
      *dw1 = desc->phys_addr;
      *dw2 = desc->irq;
      *reply = L4_IPC_SHORT_MSG;
    }
}

// allocate shared memory with driver for rx/tx lists
static void
e1000_init(emu_desc_t *desc)
{
  l4dm_mem_addr_t addrs[1];
  l4_size_t size, log2_size;
  e1000_shared_area_t *shar;
  e1000_shared_own_t  *own;

  size      = l4_round_page(sizeof(e1000_shared_area_t));
  log2_size = l4util_log2(size);
  if ((1 << log2_size) < size)
    log2_size++;
  size = 1 << log2_size;
  desc->private_mem1 =
    (l4_addr_t)l4dm_mem_allocate_named(size, L4DM_CONTIGUOUS |
					     L4DM_PINNED |
					     L4RM_LOG2_ALIGNED |
					     L4RM_MAP,
					     "e1000 rx/tx buffer");
  if (desc->private_mem1 == 0)
    {
      printf("Error allocating e1000 shared area\n");
      return;
    }
  memset((void*)desc->private_mem1, 0xff, size);
  if (l4dm_mem_phys_addr((void*)desc->private_mem1, size, addrs, 1, &size) < 0)
    {
      printf("Error determining phys addr of e1000 shared area\n");
      return;
    }
  if (size < sizeof(e1000_shared_area_t))
    {
      printf("Psize %08x < sizeof(e1000_shared_area_t)\n", size);
      return;
    }
  desc->private_mem1_phys = addrs[0].addr;
  desc->private_mem1_log2size = log2_size;
  printf("Rx/Tx buffers for %s at virt=%08x (phys=%08x)\n",
         desc->name, (l4_addr_t)desc->private_mem1, addrs[0].addr);

  size      = l4_round_page(sizeof(e1000_shared_own_t));
  log2_size = l4util_log2(size);
  if ((1 << log2_size) < size)
    log2_size++;
  size = 1 << log2_size;
  desc->private_mem2 =
    (l4_addr_t)l4dm_mem_allocate_named(size, L4DM_PINNED |
					     L4RM_LOG2_ALIGNED |
					     L4RM_MAP,
					     "e1000 rx/tx own bitmap");
  if (desc->private_mem2 == 0)
    {
      printf("Error allocating %s owner bitmap\n", desc->name);
      return;
    }
  memset((void*)desc->private_mem2, 0xff, size);
  desc->private_mem2_log2size = log2_size;
  printf("Rx/Tx owner bitmap for %s at virt=%08x\n", 
         desc->name, (l4_addr_t)desc->private_mem2);

  // for fast translation between physical/virtual addresses of private_mem
  desc->private_virt1_to_phys = (l4_addr_t)desc->private_mem1_phys
			      - (l4_addr_t)desc->private_mem1;

  shar = (e1000_shared_area_t*)desc->private_mem1;
  memset(&shar->rx_desc, 0, sizeof(shar->rx_desc));
  memset(&shar->tx_desc, 0, sizeof(shar->tx_desc));
  rx.dev_list = shar->rx_desc;
  tx.dev_list = shar->tx_desc;

  own  = (e1000_shared_own_t*)desc->private_mem2;
  memset(&own->rx_bitmap, 0, sizeof(own->rx_bitmap));
  memset(&own->tx_bitmap, 0, sizeof(own->tx_bitmap));
  rx.owner_map = own->rx_bitmap;
  tx.owner_map = own->tx_bitmap;
  rx.dev_entries = LIST_SIZE_RX;
  tx.dev_entries = LIST_SIZE_TX;
  rx.ctl = tx.ctl = 0;
}

static emu_desc_t emu_e1000 =
{
  .phys_addr = 0xff8c0000,
  .phys_size = 0x20000,
  .irq       = 5,
  .init      = e1000_init,
  .handle    = e1000_handle,
  .spec      = e1000_spec,
  .name      = "e1000"
};

static void
e1000_register(void)
{
//  emulate_register(&emu_e1000, 3);
}

L4C_CTOR(e1000_register, L4CTOR_AFTER_BACKEND);
