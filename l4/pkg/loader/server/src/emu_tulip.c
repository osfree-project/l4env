/* Some experimental code for emulating mmio of Tulip cards */

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
#include <l4/util/bitops.h>
#include <l4/util/irq.h>
#include <l4/util/port_io.h>
#include <l4/dm_mem/dm_mem.h>

#define LIST_SIZE  64

// Device rx/tx descriptor
typedef struct
{
  volatile l4_uint32_t status;
  l4_uint32_t des1;
  l4_uint32_t des2;
  l4_uint32_t des3;
} tulip_desc_t;

// Descriptors are shared ro with driver.
typedef struct
{
  tulip_desc_t rx_desc[LIST_SIZE];
  tulip_desc_t tx_desc[LIST_SIZE];
} tulip_shared_area_t;

// Owner bitmaps are shared rw with driver.
typedef struct
{
  l4_uint8_t rx_bitmap[8];
  l4_uint8_t tx_bitmap[8];
} tulip_shared_own_t;

// Local data not shared with driver.
typedef struct
{
  tulip_desc_t *drv_list;
  tulip_desc_t *dev_list;
  tulip_desc_t *drv_current;
  tulip_desc_t *dev_current;
  l4_uint8_t   *owner_map;
} tulip_list_t;

static tulip_list_t rx, tx;
static l4_uint32_t  csr6;

#define TULIP_CHAINED(d)	 ((d)->des1   &  0x01000000)
#define TULIP_EOL(d)		 ((d)->des1   &  0x02000000)
#define TULIP_SIZE1(d)		 ((d)->des1   &  0x000007ff)
#define TULIP_SIZE2(d)		(((d)->des1   &  0x003ff800) >> 11)
#define TULIP_DEVICE_OWN(d)	 ((d)->status &  0x80000000)
#define TULIP_SET_EOL(d)	 ((d)->des1   |= 0x02000000)
#define TULIP_RX_ACTIVE(d)	 ((d) & 0x0002)
#define TULIP_TX_ACTIVE(d)	 ((d) & 0x2000)

static inline void
tulip_outl(emu_desc_t *desc, unsigned value, unsigned offs)
{
  l4util_out32(value, desc->phys_addr+offs);
}

static inline unsigned
tulip_inl(emu_desc_t *desc, unsigned offs)
{
  return l4util_in32(desc->phys_addr+offs);
}

static inline void
tulip_set_list(emu_desc_t *desc, tulip_desc_t *d, unsigned reg)
{
  l4_uint32_t addr = desc_priv_virt_to_phys(desc, (l4_addr_t)d);
  tulip_outl(desc, addr, reg);
}

// check if memory range from addr until addr+size belongs to driver's
// address space
static void fastcall
tulip_check_mem_range(app_t *app, l4_addr_t addr, l4_size_t size)
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

static inline void
tulip_copy_drv_to_dev(app_t *app, tulip_desc_t *drv, tulip_desc_t *dev)
{
  tulip_check_mem_range(app, drv->des2, TULIP_SIZE1(drv));
  dev->des1 = drv->des1;
  dev->des2 = drv->des2;
  dev->des3 = 0;
  if (!TULIP_CHAINED(drv))
    {
      tulip_check_mem_range(app, drv->des3, TULIP_SIZE2(drv));
      dev->des3 = drv->des3;
    }
}

// Copy entry status from driver's ring list to our's and set device pointer.
static void fastcall
tulip_update_list(emu_desc_t *desc, tulip_list_t *l)
{
  tulip_desc_t *drv;
  tulip_desc_t *dev;
  l4_uint8_t bit;
  l4_uint8_t *byte;
  int i;

  if (!l->drv_list)
    return;

  drv  = l->drv_current;
  dev  = l->dev_current;
  bit  = 1 <<          ((dev - l->dev_list) % 8);
  byte = l->owner_map + (dev - l->dev_list) / 8;

  for (i=0; i<LIST_SIZE; i++)
    {
      if (drv->status != 0x80000000)
	{
	  l->drv_current = drv;
	  l->dev_current = dev;
	  return;
	}

      tulip_copy_drv_to_dev(desc->app, drv, dev);
      *byte |= bit;		// allow driver to use this descriptor
      drv->status = 0x80000001; // ensure that we don't copy this descriptor
				// again until the driver refills it
      asm volatile ("" : : : "memory");
      dev->status = 0x80000000; // enable device descriptor

      if (TULIP_EOL(drv))
	drv = l->drv_list;
      else if (TULIP_CHAINED(drv))
	drv = (tulip_desc_t*)addr_app_to_here(desc->app, drv->des3);
      else
	drv++;

      asm ("rolb %0 ; adc %4,%1" 
	  : "=rm"(bit), "=rm"(byte) 
	  :   "0"(bit),   "1"(byte), "ir"(0));
      if (drv == l->drv_list)
	{
	  TULIP_SET_EOL(dev);
	  dev  = l->dev_list;
	  bit  = 1;
	  byte = l->owner_map;
	}
      else
	dev++;
    }

  app_msg(desc->app, "update: ring > %d! drv=%08x dev=%08x",
	  LIST_SIZE, (unsigned)l->drv_list, (unsigned)l->dev_list);
  enter_kdebug("stop");
}

static void
tulip_handle(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
	     l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  int do_write = 1;

  if (ea.emu.write)
    {
      l4_umword_t offs = ea.emu.offs;
      switch (offs)
	{
	case 0x00: // CSR0: Bus mode
	  break;
	case 0x08: // CSR1: Transmit poll demand
	  tulip_update_list(desc, &tx);
	  break;
	case 0x10: // CSR2: Receive poll demand
	  tulip_update_list(desc, &rx);
	  break;
	case 0x18: // CSR3: Receive list base address
	  rx.drv_list = (tulip_desc_t*)addr_app_to_here(desc->app, value);
	  app_msg(desc->app, "\033[32mSet rx list %08x\033[m",
	      (unsigned)rx.drv_list);
	  rx.drv_current = rx.drv_list;
	  rx.dev_current = rx.dev_list;
	  if (TULIP_RX_ACTIVE(csr6))
	    tulip_update_list(desc, &rx);
	  tulip_set_list(desc, rx.dev_list, offs);
	  do_write = 0; // don't perform write
	  break;
	case 0x20: // CSR4: Transmit list base address
	  tx.drv_list = (tulip_desc_t*)addr_app_to_here(desc->app, value);
	  app_msg(desc->app, "\033[32mSet tx list %08x\033[m",
	      (unsigned)tx.drv_list);
	  tx.drv_current = tx.drv_list;
	  tx.dev_current = tx.dev_list;
	  if (TULIP_TX_ACTIVE(csr6))
	    tulip_update_list(desc, &tx);
	  tulip_set_list(desc, tx.dev_list, offs);
	  do_write = 0;
	  break;
	case 0x30: // CSR6
 	  if      ( TULIP_RX_ACTIVE(value) && !TULIP_RX_ACTIVE(csr6))
	    app_msg(desc->app, "RX +");
	  else if (!TULIP_RX_ACTIVE(value) &&  TULIP_RX_ACTIVE(csr6))
	    app_msg(desc->app, "RX -");
	  if      ( TULIP_TX_ACTIVE(value) && !TULIP_TX_ACTIVE(csr6))
	    app_msg(desc->app, "TX +");
	  else if (!TULIP_TX_ACTIVE(value) &&  TULIP_TX_ACTIVE(csr6))
	    app_msg(desc->app, "TX -");

	  if (TULIP_RX_ACTIVE(value) && !TULIP_RX_ACTIVE(csr6))
	    {
	      // driver wants to change receive state from stopped to running
	      tulip_update_list(desc, &rx);
	    }
	  if (TULIP_TX_ACTIVE(value) && !TULIP_TX_ACTIVE(csr6))
	    {
	      // driver wants to change transmit state from stopped to running
	      tulip_update_list(desc, &tx);
	    }
	  csr6 = value;
	  break;
	case 0x40: // CSR8
	case 0x50: // CSR10
	case 0x58: // CSR11
	case 0x60: // CSR12
	case 0x70: // CSR14
	case 0x78: // CSR15
	  // give message about unexpected CSR access
	  app_msg(desc->app, "CSR%d: %08x", offs >> 3, value);
	  break;
	case 0x28: // CSR5:  Interrupt acknowledge
	case 0x38: // CSR7:  Interrupt disable/enable
	case 0x48: // CSR9:  ROM
	case 0x68: // CSR13: SIA connectivity
	  // be silent
	  break;
	default:
	  app_msg(desc->app, "unknown offset %08x value %08x", offs, value);
	  break;
	}

      if (do_write)
	{
	  /* handle write request */
	  switch (ea.emu.size)
	    {
	    case 0: // 1 byte
	    case 1: // 2 bytes
	      app_msg(desc->app, "Tulip size %d byte(s)?", 1 << ea.emu.size);
	      enter_kdebug("stop");
	      break;
	    case 2: // 4 bytes
	      tulip_outl(desc, value, offs);
	      break;
	    }
	}
    }
  else
    *dw1 = tulip_inl(desc, ea.emu.offs);

  *reply = L4_IPC_SHORT_MSG;
}

// special command
static void
tulip_spec(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
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
      else
	enter_kdebug("bad value");
    }
  else if (ea.emu.size == 2)
    {
      // driver changed list, copy changed information to dev_list
      // (called after driver interrupt handler)
      tulip_update_list(desc, &rx);
      tulip_update_list(desc, &tx);
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
tulip_init(emu_desc_t *desc)
{
  l4dm_mem_addr_t addrs[1];
  l4_size_t size, log2_size;
  tulip_shared_area_t *shar;
  tulip_shared_own_t  *own;

  size      = l4_round_page(sizeof(tulip_shared_area_t));
  log2_size = l4util_log2(size);
  if ((1 << log2_size) < size)
    log2_size++;
  size = 1 << log2_size;
  desc->private_mem1 =
    (l4_addr_t)l4dm_mem_allocate_named(size, L4DM_CONTIGUOUS |
					     L4DM_PINNED |
					     L4RM_LOG2_ALIGNED |
					     L4RM_MAP,
					     "tulip rx/tx buffer");
  if (desc->private_mem1 == 0)
    {
      printf("Error allocating tulip shared area\n");
      return;
    }
  if (l4dm_mem_phys_addr((void*)desc->private_mem1, size, addrs, 1, &size) < 0)
    {
      printf("Error determining phys addr of tulip shared area\n");
      return;
    }
  if (size < sizeof(tulip_shared_area_t))
    {
      printf("Psize %08x < sizeof(tulip_shared_area_t)\n", size);
      return;
    }
  desc->private_mem1_phys = addrs[0].addr;
  desc->private_mem1_log2size = log2_size;
  printf("Rx/Tx buffers for %s at virt=%08x (phys=%08x)\n", 
         desc->name, (unsigned)desc->private_mem1, addrs[0].addr);

  size      = l4_round_page(sizeof(tulip_shared_own_t));
  log2_size = l4util_log2(size);
  if ((1 << log2_size) < size)
    log2_size++;
  desc->private_mem2 =
    (l4_addr_t)l4dm_mem_allocate_named(size, L4DM_PINNED |
					     L4RM_LOG2_ALIGNED |
					     L4RM_MAP,
					     "tulip rx/tx own bitmap");
  if (desc->private_mem2 == 0)
    {
      printf("Error allocating %s owner bitmap\n", desc->name);
      return;
    }
  desc->private_mem2_log2size = log2_size;
  printf("Rx/Tx owner bitmap for %s at virt=%08x\n", 
         desc->name, (unsigned)desc->private_mem2);

  // for fast translation between physical/virtual addresses of private_mem
  desc->private_virt1_to_phys = (l4_addr_t)desc->private_mem1_phys
			      - (l4_addr_t)desc->private_mem1;

  shar = (tulip_shared_area_t*)desc->private_mem1;
  memset(&shar->rx_desc, 0, sizeof(shar->rx_desc));
  memset(&shar->tx_desc, 0, sizeof(shar->tx_desc));
  rx.dev_list = shar->rx_desc;
  tx.dev_list = shar->tx_desc;

  own  = (tulip_shared_own_t*)desc->private_mem2;
  memset(&own->rx_bitmap, 0, sizeof(own->rx_bitmap));
  memset(&own->tx_bitmap, 0, sizeof(own->tx_bitmap));
  rx.owner_map = own->rx_bitmap;
  tx.owner_map = own->tx_bitmap;
}

static emu_desc_t emu_tulip =
{
//  .phys_addr = 0xa800,
  .phys_addr = 0xbc00,
  .phys_size = 0x100,
//  .irq       = 10,
  .irq       = 7,
  .init      = tulip_init,
  .handle    = tulip_handle,
  .spec      = tulip_spec,
  .name      = "tulip"
};

static void
tulip_register(void)
{
  emulate_register(&emu_tulip, 0);
}

L4C_CTOR(tulip_register, L4CTOR_AFTER_BACKEND);
