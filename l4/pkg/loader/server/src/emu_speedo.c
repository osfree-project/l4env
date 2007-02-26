/* Some experimental stuff for emulating mmio of Intel EEPRO100 devices.
 * Does not work and most probably will never work since the EEPRO100 cards
 * have a restricted format of the packet header (data has to follow the
 * header immediately) */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "emulate.h"
#include "emu_speedo.h"
#include "pager.h"

#include <stdio.h>

#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/rmgr/librmgr.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/util/bitops.h>

typedef struct {
    l4_uint32_t status;
    l4_uint32_t link;
    l4_uint32_t rx_buf_addr;
    l4_uint32_t count;
} speedo_rxfd;

typedef struct {
    l4_uint32_t	status;
    l4_uint32_t	link;
    l4_uint32_t	tx_desc_addr;
    l4_uint32_t	count;
    l4_uint32_t	tx_buf_addr0;
    l4_uint32_t	tx_buf_size0;
    l4_uint32_t	tx_buf_addr1;
    l4_uint32_t	tx_buf_size1;
} speedo_txfd;

typedef struct
{
  speedo_rxfd rx_desc[128]; // = 2kB (eepro100.c:RX_RING_SIZE is 64)
  speedo_txfd tx_desc[128]; // = 4kB (eepro100.c:TX_RING_SIZE is 64)
} speedo_shared_area_t;
	  
static unsigned SCBpointer;
static l4_addr_t recv_base;
static l4_addr_t recv_list;
static l4_addr_t send_list;

// allocate memory for rx/tx lists
void
speedo_init(emu_desc_t *desc)
{
  l4dm_mem_addr_t addrs[1];
  l4_size_t psize;
  
  psize = l4_round_page(sizeof(speedo_shared_area_t));
  desc->private_mem = l4dm_mem_allocate_named(psize,
					      L4DM_CONTIGUOUS |
					      L4DM_PINNED |
					      L4RM_MAP,
					      "speedo rx/tx buf");
  if (l4dm_mem_phys_addr(desc->private_mem, psize, addrs, 1, &psize) < 0)
    {
      printf("Error determining phys addr of ide_dma table\n");
      return;
    }

  if (psize < sizeof(speedo_shared_area_t))
    {
      printf("Psize %08x < sizeof(speedo_shared_area_t)\n", psize);
      return;
    }

  printf("Rx/Tx buffers for eepro100 at %08x\n", addrs[0].addr);
  desc->private_mem_phys = (void*)addrs[0].addr;
  desc->private_virt_to_phys = (l4_addr_t)desc->private_mem_phys
			     - (l4_addr_t)desc->private_mem;
}

static inline void
speedo_set_SCBpointer(emu_desc_t *desc, l4_uint32_t addr)
{
  *(l4_uint32_t*)(desc->map_addr+4) = addr;
}

static inline void
speedo_set_SCBcmd(emu_desc_t *desc, l4_uint16_t cmd)
{
  *(l4_uint16_t*)(desc->map_addr+2) = cmd;
}

// copy entries from driver`s ring list to our`s and set device pointer
static int
speedo_rebuild_rx_ring(emu_desc_t *desc, l4_uint16_t cmd)
{
  int i;
  speedo_rxfd *rx_drv  = (speedo_rxfd*)addr_app_to_here(desc->app, recv_list);
  speedo_rxfd *rx_real = ((speedo_shared_area_t*)desc->private_mem)->rx_desc;
  speedo_rxfd *rx_last = 0;
  speedo_rxfd *rx_real_beg = rx_real;
  l4_addr_t rx_buf_addr;

  if (recv_base != 0)
    {
      app_msg(desc->app, "Rx base is %08x", recv_base);
      enter_kdebug("stop");
    }

  enter_kdebug("stop");

  for (i=0; i<128; i++)
    {
//      app_msg(desc->app, "  s=%08x addr=%08x lnk=%08x",
//	  rx->status, rx->rx_buf_addr, rx->link);
      rx_buf_addr = rx_drv->rx_buf_addr;
      if (rx_buf_addr == 0xffffffff)
	rx_buf_addr = (l4_addr_t)rx_drv+16;

      if (   !addr_app_to_here(desc->app, rx_buf_addr)
	  || !addr_app_to_here(desc->app, rx_buf_addr+(rx_drv->count>>16)))
	{
	  app_msg(desc->app, "Rx #%d at %08x invalid %08x-%08x",
	     i, (unsigned)rx_drv, rx_buf_addr, rx_buf_addr+(rx_drv->count>>16));
	  enter_kdebug("stop");
	}

      *rx_real = *rx_drv;
      if (rx_last)
	rx_last->link = desc_priv_virt_to_phys(desc, (l4_addr_t)rx_real);

      if (rx_drv->link == 0 || (rx_drv->status & 0x80000000))
	{
	  // end-of-list
	  if (cmd != (l4_uint16_t)-1)
	    {
	      speedo_set_SCBpointer(desc, (l4_addr_t)rx_real_beg);
	      speedo_set_SCBcmd(desc, cmd);
	      speedo_set_SCBpointer(desc, (l4_addr_t)recv_list);
	    }
	  return 0;
	}

      rx_last = rx_real;
      rx_drv  = (speedo_rxfd*)addr_app_to_here(desc->app, rx_drv->link);
      rx_real++;
    }

  app_msg(desc->app, "RX Ring > 128!");
  return 0;
}

static int
speedo_check_tx(emu_desc_t *desc)
{
  int i;
  speedo_txfd *tx = (speedo_txfd*)addr_app_to_here(desc->app, send_list);

  for (i=0; i<128; i++)
    {

//      app_msg(desc->app, "  s=%08x addr=%08x/%08x lnk=%08x",
//	  tx->status, tx->tx_buf_addr0, tx->tx_buf_addr1, tx->link);
      if (tx->link == 0)
	{
	  // end-of-list
	  app_msg(desc->app, "  tx %d entries", i+1);
	  return 0;
	}

      tx = (speedo_txfd*)addr_app_to_here(desc->app, tx->link);
    }

  app_msg(desc->app, "TX Ring > 128!");
  return 0;
}

void
speedo_handle(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
	      l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  if (ea.emu.write)
    {
      l4_umword_t offs = ea.emu.offs;
      switch (offs)
	{
	case 0:
	  // SCBStatus
	  break;
	case 16:
	  // SCBCtrlMDI
	  break;
	case 2:
	  // SCBCmd
	    {
	      l4_umword_t rx_cmd = value & 0x0007;
	      l4_umword_t tx_cmd = value & 0x00f0;
	      app_msg(desc->app, "SCBCmd %02x", value & 0xff);
	      switch (tx_cmd)
		{
		case 0x10: // CU start
		  send_list = SCBpointer;
		  app_msg(desc->app, "SEND Start  => check ring %08x", 
		      send_list);
		  speedo_check_tx(desc);
		  break;
		case 0x20: // CU resume
		  app_msg(desc->app, "SEND Resume => check ring %08x", 
		      send_list);
		  speedo_check_tx(desc);
		  break;
		case 0x40: // CU load dump counters address
		  app_msg(desc->app, "LOAD DUMP COUNTER");
		  break;
		case 0x70: // CU dump and reset statistical counters
		  app_msg(desc->app, "DUMP AND RESET STATISTICAL COUNTERS");
		  break;
		case 0xa0: // CU static resume
		  app_msg(desc->app, "SEND Static Resume => check ring %08x",
		      send_list);
		  break;
		}
	      switch (rx_cmd)
		{
		case 0x01: // RU start
		  recv_list = SCBpointer;
		  app_msg(desc->app, "RECV Start => check ring %08x", 
		      recv_list+recv_base);
		  speedo_rebuild_rx_ring(desc, value);
		  break;
		case 0x02: // RU resume
		  app_msg(desc->app, "RECV Resume => check ring %08x", 
		      recv_list+recv_base);
		  speedo_rebuild_rx_ring(desc, value);
		  break;
		case 0x07: // RU resume no resources
		  app_msg(desc->app, "RECV Resume no_res => check ring %08x",
		      recv_list+recv_base);
		  speedo_rebuild_rx_ring(desc, value);
		  break;
		case 0x06: // RU load recevie base
		  recv_base = SCBpointer;
		  app_msg(desc->app, "LOAD RECV BASE  %08x", recv_base);
		  break;
		}
	      break;
	    case 4:
	      // SCBPointer -- save, we need it later
	      SCBpointer = value;
	      break;
	    default:
	      // ???
	      app_msg(desc->app, "addr=%08x value=%08x size=%d", 
		    offs, value, ea.emu.size);
	      break;
	    }
	}

      /* handle write request */
      switch (ea.emu.size)
	{
	case 0: // 1 byte
	  value &= 0xff;
	  *(l4_uint8_t*)(desc->map_addr+offs) = value;
	  break;
	case 1: // 2 bytes
	  value &= 0xffff;
	  *(l4_uint16_t*)(desc->map_addr+offs) = value;
	  break;
	case 2: // 4 bytes
	  *(l4_uint32_t*)(desc->map_addr+offs) = value;
	  break;
	}

      /* OK */
      *reply = L4_IPC_SHORT_MSG;
    }
  else
    {
      /* handle read request -- map 4k page read-only */
      *dw1   = 0;
      *dw2   = l4_fpage(desc->map_addr, L4_LOG2_PAGESIZE, 
			L4_FPAGE_RO, L4_FPAGE_MAP).fpage;
      *reply = L4_IPC_SHORT_FPAGE;
    }
}

void
speedo_spec(emu_desc_t *desc, emu_addr_t ea, l4_umword_t value,
	    l4_umword_t *dw1, l4_umword_t *dw2, void **reply)
{
  if (ea.emu.size == 1)
    {
      app_msg(desc->app, "Speedo map read-only");
      *dw1   = 0;
      *dw2   = l4_fpage((l4_addr_t)desc->private_mem, 
			bsr(l4_round_page(sizeof(speedo_shared_area_t))),
			L4_FPAGE_RO, L4_FPAGE_MAP).fpage;
      *reply = L4_IPC_SHORT_FPAGE;
    }
  else if (ea.emu.size == 2)
    {
      app_msg(desc->app, "recv_list=%08x, private_mem=%08x", 
	  addr_app_to_here(desc->app, recv_list), (unsigned)desc->private_mem);
      enter_kdebug("after interrupt");
      speedo_rebuild_rx_ring(desc, -1);
      *reply = L4_IPC_SHORT_MSG;
    }
}

