/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "ike_connector.h"

#include <l4/names/libnames.h>
#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>
#include <l4/cxx/atomic.h>

#include <l4/nethub/cfg-types.h>

#include "ip.h"
#include "sadb.h"

Ike_notify_buffer::Ike_notify_buffer()
  : free(_buf)
{
  for (unsigned i = 0; i < Num_elements; ++i)
    _buf[i].exp.type.type = NH_SADB_NOTIFY_EMPTY;
#if 0
  L4::cout << "Initialize notify buffer at 0x" << L4::hex
           << this << L4::dec << " with " << Num_elements << " elements\n"
           << "(element size " << sizeof(Element) << " bytes, last ptr @"
	   << L4::hex << &free << L4::dec << ").\n";
#endif
}

Ike_notify_buffer::Element *
Ike_notify_buffer::get_element()
{
  Element *mine;
  do
    {
      mine = free;
      if (mine->exp.type.type & NH_SADB_NOTIFY_COMMITED)
	return 0; // no more entries free
      if (!L4::compare_and_swap(&mine->exp.type.type, 
                                (l4_uint16_t)NH_SADB_NOTIFY_EMPTY, 
				(l4_uint16_t)NH_SADB_NOTIFY_WIP))
	l4_thread_switch(L4_NIL_ID);
      else
	break;
    }
  while(1);

  if (free >= _buf + Num_elements-1)
    free = 0;
  else
    ++free;

  L4::cout << "IKE NOTIFY: allocated entry @" << L4::hex << mine 
           << " next is @" << free << L4::dec << '\n';
  
  return mine;
}

bool
Ike_notify_buffer::commit_element(Element *e, unsigned type)
{
  if (e->exp.type.type != NH_SADB_NOTIFY_WIP)
    return false;

  e->exp.type.type = type;
  
  L4::cout << "IKE NOTIFY: commited entry @" << L4::hex << e
           << L4::dec << " type is "  << type << '\n';
  
  return true;
}

void Ike_connector::run()
{
  L4::cout << '(' << self() << ") IKE connector is up!\n";
  names_register(_name);
}

bool 
Ike_connector::acquire(unsigned satype, Ip_packet const *p, unsigned iface)
{
  Ike_notify_buffer::Element *e = _nbuf.get_element();
  if (!e)
    {
      L4::cerr << "warning: IKE notify buffer overflow (send ACQUIRE)\n";
      return false;
    }
  
  Nh_sadb_acquire_msg *a = &e->acq;
  a->satype = satype & NH_SA_TYPE_MASK;
  a->src.proto = p->protocol();
  a->src.prefix_len = 0;
  a->src.port = 0; //p->src_port();
  a->src.address = p->saddr();
  
  a->dst.proto = p->protocol();
  a->dst.prefix_len = 0;
  a->dst.port = 0; //p->dst_port();
  a->dst.address = p->daddr();
  
  return _nbuf.commit_element(e, NH_SADB_NOTIFY_ACQUIRE);
}
  

bool 
Ike_connector::expired(SA const *sa, bool hard)
{
  Ike_notify_buffer::Element *e = _nbuf.get_element();
  if (!e)
    {
      L4::cerr << "warning: IKE notify buffer overflow (send EXPIRE)\n";
      return false;
    }
  
  Nh_sadb_expire_msg *a = &e->exp;
  a->sa.satype = sa->satype();
  a->sa.spi    = sa->spi();
  a->sa.dst_ip = sa->dst();

  SA::Lifetime const &c = sa->current();

  a->curr.bytes = c.bytes;
  a->curr.addtime = c.addtime/1000;
  a->curr.usetime = c.usetime/1000;
  a->curr.allocations = 0;

  a->expired = hard;
  
  return _nbuf.commit_element(e, NH_SADB_NOTIFY_EXPIRE);
}
