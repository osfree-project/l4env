/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/nethub/client.h>
#include <l4/nethub/base.h>

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4types.h>

int nh_recv( struct Nh_iface *iface, void *buffer, unsigned long *len )
{
  l4_umword_t id;
  l4_msgdope_t res;
  int err;

  err = l4_ipc_call( iface->in, 0, (l4_umword_t)buffer, *len | 0x80000000,
                     0, &id, (l4_umword_t*)len,
		     L4_IPC_TIMEOUT(153,7,0,0,0,0), &res );
  switch (err)
    {
    case 0:
      if (id == 0xaffe0001)
	return L4_NH_OK;
      else if (id == 0xaffe0002)
	return L4_NH_EREALLOC;
      else
	return L4_NH_ERR;
    case L4_IPC_ENOT_EXISTENT:
    case L4_IPC_SETIMEOUT:
      return L4_NH_EAGAIN;
    default:
      return L4_NH_ERR;
    }
}

int nh_send( struct Nh_iface *iface, void *buffer, void *priv )
{
  l4_msgdope_t result;
  //char empty = r->empty;
  struct Nh_packet_ring_entry *e;
  e = &iface->out_ring->ring[iface->out_ring->next];
  
  if (e->flags & 1)
    return L4_NH_ERR;

  e->pkt = buffer;
  e->priv = priv;
  e->flags = 1;

  iface->out_ring->next = (iface->out_ring->next +1) % 32;

//  if (empty)
    l4_ipc_send( iface->out, 0, 0, 0, L4_IPC_SEND_TIMEOUT_0, &result);

  return L4_NH_OK;

}

int nh_region_mapper( void const *start, void const *end )
{
  l4_umword_t magic, address;
  L4::MsgDope result;
  l4_threadid_t th;
  int err;
  l4_fpage_t fp;
  while (1)
    {
      err = l4_ipc_wait( &th, 0, &magic, &address, L4_IPC_NEVER, 
	                 &((l4_msgdope_t&)result) );
      while (!err)
	{
	  if (magic != 0xaffec00d) 
	    {
	      L4::cout << "nethub (mapper): got request with wrong opcode\n";
	      break;
	    }
	  if (address < (l4_umword_t)start || address >=(l4_umword_t)end)
	    {
	      L4::cout << "nethub (mapper): requested address is out of range\n"
		       << "  " << L4::hex << address << " not in [" << start 
		       << ", " << end << ")\n";
	      fp.raw = 0;
	    }
	  else
	    {
	      fp = l4_fpage( address & ~0x0fff, 12, L4_FPAGE_RW, L4_FPAGE_MAP );
	    }
	  err = l4_ipc_reply_and_wait( th, (void*)2, 0, fp.raw,
	                               &th, 0, &magic, &address,
				       L4_IPC_NEVER, &((l4_msgdope_t&)result) );
	  
	}
    }
  return 0;
}

void nh_for_each_empty_slot( struct Nh_packet_ring *r, Nh_slot_func *f )
{
  unsigned x = 0;
  for (;x<32; ++x)
    {
      if(!(r->ring[x].flags & 1))
	f(&r->ring[x].priv);
    }
}

