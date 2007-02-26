/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#include "interface.h"
#include <l4/sys/ipc.h>
#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>
#include <l4/cxx/l4types.h>

void Tx_iface::xmit( Iface *i, unsigned slot )
{
  int err;
  l4_msgdope_t r;
  
  err = l4_ipc_send( peer(), 0, (l4_umword_t)i, slot, L4_IPC_NEVER, &r );
  if (err)
    L4::cout << "IPC error from router to worker: " 
             << (L4::MsgDope)r << '\n';

}

void Iface::tx_empty()
{
  int err;
  l4_msgdope_t r;
  
  err = l4_ipc_send( txe_irq, 0, 0, 0, L4_IPC_BOTH_TIMEOUT_0, &r );
}

#if 0
void Iface::xmit_pkt( Rx_pkt const &pkt, Routing_entry *re ) 
{
  //  L4::cout << "send packet to worker\n";
  tx_if.set(pkt,re);
  
  int err;
  l4_msgdope_t result;
  //  L4::cout << "send pkt to worker: " << tx_if.peer() << '\n';
  err = l4_ipc_send( tx_if.peer(), 0, 
                     (l4_umword_t)this, 0, L4_IPC_NEVER, &result );
  if (err)
    L4::cout << "IPC error from router to worker: " 
             << (L4::MsgDope)result << '\n';
}
#endif
