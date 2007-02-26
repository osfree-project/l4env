/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "interface.h"
#include "routing_fab.h"

#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/cxx/iostream.h>

void Routing_fab::run()
{
  // L4::cout << "Starting router fabric\n";
  if (!rtab)
    {
      L4::cerr << "FATAL: routing fabric has invalid routing table\n"
	          "       stop routing fabric\n";
      return;
    }
  
  while (1)
    {
      Rx_pkt in;
      Iface *in_if;
      l4_threadid_t other;
      l4_msgdope_t result;
      l4_umword_t d1, d2;

      // L4::cout << "router check for incoming packets\n";
      while ((in_if = ifl->next_active(d1)))
	{
	  // L4::cout << "found active interface\n";
	  unsigned slot;
	  in = in_if->next_to_handle(slot);
	  if (in.valid())
	    {
	      // L4::cout << "route pkt from iface " << d1 << '\n';
	      Routing_entry *re = rtab->route(d1, in.packet());
	      if (!re)
		{
		  /*
		  L4::cout << "no route for packet: [if=" << d1
		           << "; " << Ip_addr(in.packet()->saddr(),(u32)-1) 
			   << "->" << Ip_addr(in.packet()->daddr(),(u32)-1) 
			   << "]\n";
	          */
		  // drop this packet
		  in.clear_flags();
		  in_if->inc_rx_drop_count();
		}
	      else
		{
		  // L4::cout << " transmit slot " << slot 
		  //          << " re=" << re << '\n';
		  in_if->set_route( slot, re );
 		  re->destination()->xmit(in_if, slot);
		}
	    }
	}
      
      // wait for notification
      l4_ipc_wait( &other, 0, &d1, &d2, L4_IPC_NEVER, &result );
    }
}

void Routing_fab::unresolved_fault( void *priv )
{
  L4::cout << "Unresolved Fault -> killing interface and restarting router\n";
  Iface *i = (Iface*)priv;
  if (i) i->invalidate();
  stop();
  start();
}

