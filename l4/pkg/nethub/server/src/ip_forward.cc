/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "ip_forward.h"
#include "ip.h"

#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>
#include <l4/cxx/l4types.h>
#include <l4/util/util.h>
#include "interface.h"
#include "routing.h"

#include <stddef.h>


class Ip_address 
{
public:
  Ip_address(u32 a) : a(a) {}
  void print( L4::BasicOStream &s ) const
  {
    s << L4::dec << (a >> 24) << '.' << ((a >> 16) & 0xff) << '.'
      << ((a >> 8) & 0xff) << '.' << (a & 0xff);
  }
  u32 a;
};

L4::BasicOStream &operator << (L4::BasicOStream &s, Ip_address const &a)
{
  a.print(s);
  return s;
}

u32 fwd_threads = 0;

void Ip_fwd::print(L4::BasicOStream &s) const
{
  s << "IP forward: " << name();
}

char const *const Ip_fwd::name() const
{
  return "plain";
}

inline unsigned next_thread()
{
  unsigned t;
  for (t = 0; t<32 && ((1<<t) & fwd_threads); t++);

  if (t<32) 
    {
      fwd_threads |= 1<<t;
      //L4::cout << "Allocate fwd thread: " << t+10 << "\n";
      return t+10;
    }
  else
    {
      L4::cerr << "error: no more fwd threads\n";
      return 0;
    }
}

inline void free_thread( unsigned t )
{
  fwd_threads &= ~(1<<(t-10));
}

Ip_fwd::Ip_fwd( Iface *out) 
  : Thread( stack + 1023, next_thread() ), 
    _rcv(L4_INVALID_ID), _snd(L4_INVALID_ID),
    test_checksum(0), out_iface(out)
{}

Ip_fwd::~Ip_fwd()
{
  free_thread( self().id.lthread );
}


struct Message
{
  l4_umword_t w[2];
  unsigned rx_slot() const { return w[1]; }
  Iface *rx_iface() const { return (Iface*)w[0]; }
  
  unsigned long tx_len() const { return w[1]; }
  Ip_packet *tx_pkt() const { return (Ip_packet*)w[0]; }
};

void  Ip_fwd::run() 
{
  Message m;
  Ip_packet *in = 0, *out = 0;
  Rx_pkt in_pkt;
  Iface *in_if = 0;
  Routing_entry *route = 0;
  l4_threadid_t snd, other;
  l4_msgdope_t result;
  int err;

  if (out_iface->region()->is_empty())
    {
      L4::cerr << "error: (" << self() 
	       << ") :someone started me with an empty "
	          "shmem region\n        --> stopping myself\n";
      shutdown();
    }

  while(1) 
    {
      if (!in_pkt.valid()) 
	{
          while((err = l4_ipc_wait( &other, 0, 
	                            &m.w[0], &m.w[1], 
	  			    L4_IPC_NEVER, 
				    &result )))
	    {
	      L4::cerr << "warning: " << (L4::MsgDope)result << '\n';
	      l4_sleep(1000);
	    }
        }
      else
	{
	  while((err = l4_ipc_receive( _snd, 0, 
	                               &m.w[0], &m.w[1], 
	  			       L4_IPC_NEVER, 
				       &result )))
	    {
	      L4::cerr << "warning: " << (L4::MsgDope)result << '\n';
	      l4_sleep(1000);
	    }

	  other = _snd;

	}

      if(out == 0 && _snd == other && (m.tx_len() & 0x80000000) ) 
	{
	  // got receive buffer from client
	 
	  // L4::cout << self() << " got recv buffer from " << other << '\n';
	  unsigned long len = m.tx_len() & ~0x80000000;

	  // test if at least IP header fits into size
	  if (len < 20) 
	    {
	      L4::cerr << "warning: got too small buffer for receive\n"
		          "         just try it again\n";
	      err = l4_ipc_send( other, 0, 0, out->len(), 
		                 L4_IPC_BOTH_TIMEOUT_0, 
		                 &result );
	      continue;
	    }

	  snd = other;
	  out = out_iface->region()->localize(m.tx_pkt());
	  if (!out)
	    {
	      L4::cerr << '(' << self() 
		       << "): Invalid output buffer -> killing myself\n";
	      out_iface->invalidate();
	      shutdown();
	    }
	  out->len( len );
	}
     
      if (!in_pkt.valid() && _rcv == other)
	{
          // got packet from routing fabric
	  
	  // L4::cout << self() << " got packet to send from " << other << '\n';
	  in_if= m.rx_iface();
	  unsigned rx_slot = m.rx_slot();
	  
	  in_pkt = in_if->packet(rx_slot);
	  route = in_if->get_route(rx_slot);
	  
	  in = in_pkt.packet();

	  // again at least the header must be valid
	  if(in->head_len() < 5 || in->version()!=4) 
	    {
	      in_pkt.clear_flags();
	      L4::cerr << "waring: (" << self() 
		       << ") wrong IP version or header size\n";
	      in_pkt.invalidate();
	      out_iface->tx_empty();
	      out_iface->inc_tx_drop_count();
	    }
	  else
	    {
#if 0
	  L4::cout << "IN: saddr=" << (Ip_address)Net::n_to_h(in->saddr()) 
	           << " daddr=" << (Ip_address)Net::n_to_h(in->daddr())
		   << " proto=" << (unsigned)in->protocol()
		   << " len=" << (unsigned)in->len()
		   << " (" << (unsigned)in->is_fragment() << ")\n";
#endif
	
	      if(test_checksum && !in->check_checksum()) 
		{
		  in_pkt.clear_flags();
		  L4::cerr << "warning: (" << self() << ") bad IP checksum\n";
		  in_pkt.invalidate();
		  out_iface->tx_empty();
	          out_iface->inc_tx_drop_count();
		}
	    }
	}
#if 0
      if( in && !out )
	in_if->set_empty(); // not empty
#endif
      
      if(in_pkt.valid() && out) 
	{
	  l4_msgdope_t r_s, r_r;
	  r_s.raw = 0;
	  r_r.raw = 0;
	  switch (forward( route, in, out )) 
	    {
	    case fwd_ok:
	      in_pkt.clear_flags();
#if 0
	      L4::cout << self() << " send pkt: saddr="
		       << (Ip_address)(Net::n_to_h(out->saddr()))
		       << " daddr=" << (Ip_address)(Net::n_to_h(out->daddr()))
		       << "\n";
#endif
	      l4_ipc_send( snd, 0, 0xaffe0001, out->len(), 
		           L4_IPC_BOTH_TIMEOUT_0, 
		           &r_r );
	      out = 0;
	      in_pkt.invalidate();
	      out_iface->tx_empty();
	      out_iface->inc_tx_count();
	      break;
	    case fwd_out_size_error:
	      // L4::cout << "rsize need=" 
	      //          << (unsigned)out->len() << "\n";
	      l4_ipc_send( snd, 0, 0xaffe0002, out->len(), 
		           L4_IPC_BOTH_TIMEOUT_0, 
		           &r_r );
	      out = 0;
	      break;

	    default:
	    case fwd_retry:
	    case fwd_drop:
	      in_pkt.clear_flags();
	      in_pkt.invalidate();
	      out_iface->tx_empty();
	      out_iface->inc_tx_drop_count();
	      break;
	    } 

	  if (L4_IPC_IS_ERROR(r_r))
	    L4::cerr << "ERROR: answer to receiver: " << (L4::MsgDope)r_r 
		     << '\n';

	  continue;
	}
    }
}

Ip_fwd::Fwd_result Ip_fwd::forward( Routing_entry *, 
                                    Ip_packet *in, Ip_packet *out)
{
  unsigned long *src, *dst;
  src = reinterpret_cast<unsigned long*>(in);
  dst = reinterpret_cast<unsigned long*>(out);
  int len = in->len();
  if(out->len() < len)
    { 
      out->len(len);
      return fwd_out_size_error;
    }

  if (in->ttl()<=1)
    {
      L4::cerr << "waring: drop packet due to ttl underrun\n";
      return fwd_drop;
    }

  for(; len > 0; len -= sizeof(unsigned long))
    *dst++ = *src++;

  out->ttl( in->ttl()-1 );
  out->set_checksum();

  return fwd_ok;
}

