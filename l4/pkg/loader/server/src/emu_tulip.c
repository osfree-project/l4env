/* Some experimental code for emulating mmio of Tulip cards */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "emulate.h"
#include "emu_tulip.h"
#include "pager.h"

#include <stdio.h>

#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/rmgr/librmgr.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/bitops.h>
#include <l4/sys/ktrace.h>

#define EMU_RECV
#define EMU_SEND

#define RX_LIST_SIZE	64
#define TX_LIST_SIZE	64

typedef struct {
    volatile l4_uint32_t status;
    l4_uint32_t des1;
    l4_uint32_t buf;
    l4_uint32_t next;
} tulip_desc;

typedef struct {
    tulip_desc rx_desc[RX_LIST_SIZE];
    tulip_desc tx_desc[TX_LIST_SIZE];
    unsigned   rx_desc_copied_to_driver;
    unsigned   tx_desc_copied_to_device;
} tulip_shared_area_t;

static l4_addr_t recv_list;
static l4_addr_t send_list;
static l4_uint32_t csr6;

static tulip_desc *rx_real_current;  // current desc in phys rx list
static tulip_desc *rx_drv_current;   // current desc in virt rx list
static tulip_desc *tx_real_current;  // current desc in phys tx list
static tulip_desc *tx_drv_current;   // current desc in virt tx list

#define TULIP_CHAINED(d)	 (d->des1   & 0x01000000)
#define TULIP_EOL(d)		 (d->des1   & 0x02000000)
#define TULIP_SIZE1(d)		 (d->des1   & 0x000007ff)
#define TULIP_SIZE2(d)		((d->des1   & 0x003ff800) >> 11)
#define TULIP_DEVICE_OWN(d)	 (d->status & 0x80000000)

static unsigned inline
sync_cli(void)
{
  unsigned flags;
  asm volatile ("pushf ; pop %%eax ; cli" : "=a"(flags));
  return flags;
}

static void inline
sync_sti(unsigned flags)
{
  asm volatile ("push %%eax ; popf" : : "a"(flags));
}

static inline void
tulip_outl(emu_desc_t *desc, unsigned value, unsigned offs)
{
  asm volatile ("out %0, %w1" : : "a"(value), "Nd"(offs+desc->phys_addr));
}

static inline void
tulip_set_recv_list(emu_desc_t *desc)
{
  tulip_shared_area_t *shar = (tulip_shared_area_t*)desc->private_mem;
  tulip_outl(desc, 
	     desc_priv_virt_to_phys(desc, (l4_addr_t)shar->rx_desc), 0x18);
}

static inline void
tulip_set_send_list(emu_desc_t *desc)
{
  tulip_shared_area_t *shar = (tulip_shared_area_t*)desc->private_mem;
  tulip_outl(desc, 
	     desc_priv_virt_to_phys(desc, (l4_addr_t)shar->tx_desc), 0x20);
}

// allocate shared memory with driver for rx/tx lists
void
tulip_init(emu_desc_t *desc)
{
  l4dm_mem_addr_t addrs[1];
  l4_size_t psize;
  
  psize = l4_round_page(sizeof(tulip_shared_area_t));
  desc->private_mem = l4dm_mem_allocate_named(psize,
					      L4DM_CONTIGUOUS |
					      L4DM_PINNED |
					      L4RM_MAP,
					      "tulip rx/tx buf");
  if (l4dm_mem_phys_addr(desc->private_mem, psize, addrs, 1, &psize) < 0)
    {
      printf("Error determining phys addr of ide_dma table\n");
      return;
    }

  if (psize < sizeof(tulip_shared_area_t))
    {
      printf("Psize %08x < sizeof(tulip_shared_area_t)\n", psize);
      return;
    }

  printf("Rx/Tx buffers for tulip at virt=%08x phys=%08x\n", 
         (unsigned)desc->private_mem, addrs[0].addr);
  desc->private_mem_phys     = (void*)addrs[0].addr;

  // for fast translating between physical/virtual addresses of private_mem
  desc->private_virt_to_phys = (l4_addr_t)desc->private_mem_phys
			     - (l4_addr_t)desc->private_mem;
}

// check if memory range from addr until addr+size belongs to driver's
// address space
static void __attribute__((regparm(3)))
tulip_check_mem_range(app_t *app, l4_addr_t addr, l4_size_t size)
{
  // only check if receive engine is active else the receise list
  // could still be uninitialized
  // only check if size is != 0 else address is not used
  if ((csr6 & 2) && (size || addr))
    {
      if (   !addr_app_to_here(app, addr)
	  || !addr_app_to_here(app, addr+size))
	{
	  app_msg(app, "Rx/Tx at %08x-%08x invalid", addr, addr+size);
	  enter_kdebug("FATAL");
	}
    }
}

// copy entries from driver's descriptor list to our
static void
tulip_init_rx_list(emu_desc_t *desc)
{
#ifdef EMU_RECV
  tulip_desc *rx_drv, *rx_real, *rx_last = 0;
  tulip_shared_area_t *shar = (tulip_shared_area_t*)desc->private_mem;
  int i;

  if (!recv_list)
    return;

  rx_drv_current  = 
  rx_drv          = (tulip_desc*)addr_app_to_here(desc->app, recv_list);
  rx_real_current = 
  rx_real         = shar->rx_desc;

  shar->rx_desc_copied_to_driver = 0;

  for (i=0; i<RX_LIST_SIZE; i++)
    {
      tulip_check_mem_range(desc->app, rx_drv->buf, TULIP_SIZE1(rx_drv));
      rx_real->des1 = rx_drv->des1;
      rx_real->buf  = rx_drv->buf;
      rx_real->next = 0;

      if (TULIP_CHAINED(rx_drv))
	{
	  if (rx_last)
	    rx_last->next = desc_priv_virt_to_phys(desc, (l4_addr_t)rx_real);
	}
      else
      	{
	  tulip_check_mem_range(desc->app, rx_drv->next, TULIP_SIZE2(rx_drv));
     	  rx_real->next = rx_drv->next;
	}

      // driver wants to own the decriptor to the device
      if (TULIP_DEVICE_OWN(rx_drv) && (!TULIP_DEVICE_OWN(rx_real)))
	rx_real->status = 0x80000000;
 
      if (TULIP_EOL(rx_drv))
	return;

      // next entry in list
      if (TULIP_CHAINED(rx_drv))
	{
	  rx_last = rx_real;
	  rx_drv  = (tulip_desc*)addr_app_to_here(desc->app, rx_drv->next);
	}
      else
	rx_drv++;
      rx_real++;
    }

  app_msg(desc->app, "rx_init: RX Ring > %d! drv=%08x real=%08x", 
	  RX_LIST_SIZE,
	  (unsigned)(tulip_desc*)addr_app_to_here(desc->app, recv_list),
	  (unsigned)shar->rx_desc);
  enter_kdebug("stop");
#endif
}

// Copy entry status from driver`s ring list to our`s and set device
// pointer.
static void
tulip_update_rx_list(emu_desc_t *desc)
{
#ifdef EMU_RECV
  tulip_shared_area_t *shar = (tulip_shared_area_t*)desc->private_mem;
  tulip_desc *rx_drv  = rx_drv_current;
  tulip_desc *rx_real = rx_real_current;
//  int real_start      = rx_real - shar->rx_desc;
  int i, count        = shar->rx_desc_copied_to_driver;

  if (!recv_list)
    return;

  if (count > 8) // XXX hard-coded sanity check
    {
      app_msg(desc->app, "More than 8 descriptors (%d) to copy", count);
      enter_kdebug("stop");
    }

  for (i=0; i<RX_LIST_SIZE; i++)
    {
      if (i == count)
	{
	  unsigned flags;
//	  app_msg(desc->app, "  updated %d entries from #%d to #%d", 
//	      count, real_start, i);
	  rx_drv_current  = rx_drv;
	  rx_real_current = rx_real;

	  flags = sync_cli();
	  shar->rx_desc_copied_to_driver -= count;
	  sync_sti(flags);

	  return;
	}

      tulip_check_mem_range(desc->app, rx_drv->buf, TULIP_SIZE1(rx_drv));
      rx_real->des1 = rx_drv->des1;
      rx_real->buf  = rx_drv->buf;
      if (!TULIP_CHAINED(rx_drv))
      	{
	  tulip_check_mem_range(desc->app, rx_drv->next, TULIP_SIZE2(rx_drv));
     	  rx_real->next = rx_drv->next;
	}

      // driver wants to own the decriptor to the device
      if (TULIP_DEVICE_OWN(rx_drv) && (!TULIP_DEVICE_OWN(rx_real)))
	rx_real->status = 0x80000000;

      // next entry
      if (TULIP_EOL(rx_drv))
	{
	  rx_drv  = (tulip_desc*)addr_app_to_here(desc->app, recv_list);
	  rx_real = shar->rx_desc;
	}
      else if (TULIP_CHAINED(rx_drv))
	{
	  rx_drv  = (tulip_desc*)addr_app_to_here(desc->app, rx_drv->next);
	  rx_real++;
	}
      else
	{
	  rx_drv++;
	  rx_real++;
	}
    }

  app_msg(desc->app, "rx_update: RX Ring > %d! drv=%08x real=%08x", 
	  RX_LIST_SIZE,
	  (unsigned)(tulip_desc*)addr_app_to_here(desc->app, recv_list),
	  (unsigned)shar->rx_desc);
  enter_kdebug("stop");
#endif
}

// copy entries from driver's descriptor list to our
static void
tulip_init_tx_list(emu_desc_t *desc)
{
#ifdef EMU_SEND
  tulip_desc *tx_drv, *tx_real, *tx_last = 0;
  tulip_shared_area_t *shar = (tulip_shared_area_t*)desc->private_mem;
  int i;

  if (!send_list)
    return;

  tx_drv_current  = 
  tx_drv          = (tulip_desc*)addr_app_to_here(desc->app, send_list);
  tx_real_current = 
  tx_real         = shar->tx_desc;

  shar->tx_desc_copied_to_device = 0;

  for (i=0; i<RX_LIST_SIZE; i++)
    {
      tulip_check_mem_range(desc->app, tx_drv->buf, TULIP_SIZE1(tx_drv));
      tx_real->des1 = tx_drv->des1;
      tx_real->buf  = tx_drv->buf;
      tx_real->next = 0;

      if (TULIP_CHAINED(tx_drv))
	{
	  if (tx_last)
	    tx_last->next = desc_priv_virt_to_phys(desc, (l4_addr_t)tx_real);
	}
      else
      	{
	  tulip_check_mem_range(desc->app, tx_drv->next, TULIP_SIZE2(tx_drv));
     	  tx_real->next = tx_drv->next;
	}

      // driver wants to own the decriptor to the device
      if (TULIP_DEVICE_OWN(tx_drv) && (!TULIP_DEVICE_OWN(tx_real)))
	tx_real->status = tx_drv->status;
 
      if (TULIP_EOL(tx_drv))
	return;

      // next entry in list
      if (TULIP_CHAINED(tx_drv))
	{
	  tx_last = tx_real;
	  tx_drv  = (tulip_desc*)addr_app_to_here(desc->app, tx_drv->next);
	}
      else
	tx_drv++;
      tx_real++;
    }

  app_msg(desc->app, "tx_init: TX Ring > %d! drv=%08x real=%08x", 
	  TX_LIST_SIZE,
	  (unsigned)(tulip_desc*)addr_app_to_here(desc->app, send_list),
	  (unsigned)shar->tx_desc);
  enter_kdebug("stop");
#endif
}

static void
tulip_update_tx_list(emu_desc_t *desc)
{
#ifdef EMU_SEND
  tulip_shared_area_t *shar = (tulip_shared_area_t*)desc->private_mem;
  tulip_desc *tx_drv  = tx_drv_current;
  tulip_desc *tx_real = tx_real_current;
//  int real_start      = tx_real - shar->tx_desc;
  int i;

  if (!send_list)
    return;

  for (i=0; i<TX_LIST_SIZE; i++)
    {
      if (!TULIP_DEVICE_OWN(tx_drv))
	{
	  unsigned flags;
//	  app_msg(desc->app, "  updated %d entries from #%d to #%d", 
//	      count, real_start, i);
	  tx_drv_current  = tx_drv;
	  tx_real_current = tx_real;

	  flags = sync_cli();
	  shar->tx_desc_copied_to_device += i;
	  sync_sti(flags);

	  return;
	}

      tulip_check_mem_range(desc->app, tx_drv->buf, TULIP_SIZE1(tx_drv));
      tx_real->des1 = tx_drv->des1;
      tx_real->buf  = tx_drv->buf;
      if (!TULIP_CHAINED(tx_drv))
      	{
	  tulip_check_mem_range(desc->app, tx_drv->next, TULIP_SIZE2(tx_drv));
     	  tx_real->next = tx_drv->next;
	}

      // driver wants to own the decriptor to the device
      if (TULIP_DEVICE_OWN(tx_drv) && (!TULIP_DEVICE_OWN(tx_real)))
	tx_real->status = tx_drv->status;

      // next entry
      if (TULIP_EOL(tx_drv))
	{
	  tx_drv  = (tulip_desc*)addr_app_to_here(desc->app, send_list);
	  tx_real = shar->tx_desc;
	}
      else if (TULIP_CHAINED(tx_drv))
	{
	  tx_drv  = (tulip_desc*)addr_app_to_here(desc->app, tx_drv->next);
	  tx_real++;
	}
      else
	{
	  tx_drv++;
	  tx_real++;
	}
    }

  app_msg(desc->app, "tx_update: TX Ring > %d! drv=%08x real=%08x", 
	  TX_LIST_SIZE,
	  (unsigned)(tulip_desc*)addr_app_to_here(desc->app, send_list),
	  (unsigned)shar->tx_desc);
  enter_kdebug("stop");
#endif
}

void
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
	  tulip_update_tx_list(desc);
	  break;
	case 0x10: // CSR2: Receive poll demand
	  tulip_update_rx_list(desc);
	  break;
	case 0x18: // CSR3: Receive list base address
	  app_msg(desc->app, "Set receive list");
	  recv_list = value;
#ifdef EMU_RECV
	  tulip_init_rx_list(desc);
	  tulip_set_recv_list(desc);
	  do_write = 0; // don't perform write
#endif
	  break;
	case 0x20: // CSR4: Receive list base address
	  app_msg(desc->app, "Set send list");
	  send_list = value;
#ifdef EMU_SEND
	  tulip_init_tx_list(desc);
	  tulip_set_send_list(desc);
	  do_write = 0;
#endif
	  break;
	case 0x30: // CSR6
	  if ((value & 0x0002) && !(csr6 & 0x0002))
	    {
	      // driver wants to change receive state from stopped to running
	      tulip_update_rx_list(desc);
	    }
	  if ((value & 0x2000) && !(csr6 & 0x2000))
	    {
	      // driver wants to change transmit state from stopped to running
	      tulip_update_tx_list(desc);
	    }
	  csr6 = value;
	  break;
	case 0x40:
	case 0x50:
	case 0x58:
	case 0x60:
	case 0x70:
	case 0x78:
	  // give message about not expected CSR access
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
	      value &= 0xff;
	      app_msg(desc->app, "Tulip size %d byte(s)?", 1 << ea.emu.size);
	      enter_kdebug("stop");
	      break;
	    case 2: // 4 bytes
	      tulip_outl(desc, value, offs);
	      break;
	    }
	}

      /* OK */
      *reply = L4_IPC_SHORT_MSG;
    }
  else
    {
      app_msg(desc->app, "Tulip read?");
    }
}

// special command
void
tulip_spec(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
	   l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  if (ea.emu.size == 1)
    {
      *dw1   = 0;
      *dw2   = l4_fpage((l4_addr_t)desc->private_mem, 
			bsr(l4_round_page(sizeof(tulip_shared_area_t))),
			L4_FPAGE_RW /*XXX*/, L4_FPAGE_MAP).fpage;
      *reply = L4_IPC_SHORT_FPAGE;
    }
  else if (ea.emu.size == 2)
    {
      // driver changed list, copy changed information to real_list
      // (called after driver interrupt handler)
      tulip_update_rx_list(desc);
      *reply = L4_IPC_SHORT_MSG;
    }
}

