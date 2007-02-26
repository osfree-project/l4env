/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "sadb.h"
#include "ike_connector.h"

#include <l4/cxx/iostream.h>

SA::~SA()
{
  if (i) delete i;
  if (a) delete a;
  if (c) delete c;
}

Tx_iface *SA::destination() const
{ return iface(); }


Tx_iface *Void_sa::destination() const
{ return 0; }

unsigned Eroute_table::add(Eroute *r)
{
  if (!r)
    return 0;

  Eroute *e = get(r->src_if, r->d_mask, r->d_addr, r->s_mask, r->s_addr,
                  r->sel_mask, r->selector, r->protocol);
  if (e)
    return 0;

  *r->chain() = first;
  first = r;
  return 1;
}

Eroute *Eroute_table::get(u32 src_if, u32 daddr, u32 saddr, u64 _selector,
                          u8 _proto) const
{
  Eroute *r = first;
  while (r && !r->matches(src_if, daddr,saddr,_selector,_proto))
    r = *r->chain();

  return r;
}

Routing_entry *Eroute_table::route(unsigned iface, Ip_packet *p) const
{
  SA *sa = 0;
  if ((_if_mode >> iface) & 1)
    {
      switch(p->protocol()) 
	{
	case IP_ESP_PROTO:
	  sa = sadb->get(SA::esp, ((Ip_esp_packet*)p)->spi(), p->daddr());
	  break;
	case IP_AH_PROTO:
	  sa = sadb->get(SA::ah, ((Ip_ah_packet*)p)->spi(), p->daddr());
	  break;
	};
    }

  if (sa)
    {
      if (sa->direction == SA::dir_out)
	{
	  L4::cout << "using outbound SA for inbound traffic\n";
	  return 0;
	}
      sa->direction = SA::dir_in;
      return sa;
    }

  Eroute *r = get(iface, p, 0);
  if (r)
    {
      if (r->re()->is_void())
	{
	  Void_sa *v = (Void_sa*)(r->re());
	  ike_connector->acquire(v->type(), p, iface);
	  // XXX: must be able to set the policy to the acquired SA
	  return 0;
	}
      
      sa = r->sa();
      if (sa->state() != SA::mature && sa->state() != SA::dying)
	return 0;

      if (sa->direction == SA::dir_in)
	{
	  L4::cout << "using inbound SA for outbound traffic\n";
	  return 0;
	}
      sa->direction = SA::dir_out;
      return sa;
    }
  else
    return 0;
}

Eroute *Eroute_table::get(u32 src_if, u32 dmask, u32 daddr, 
                          u32 smask, u32 saddr,
                          u64 _sel_mask, u64 _selector, u8 _proto) const
{
  Eroute *r = first;
  while (r && !r->equals(src_if, dmask, daddr, smask, saddr, 
	                 _sel_mask, _selector, _proto))
    r = *(r->chain());

  return r;
}

unsigned Eroute_table::del(u32 src_if, u32 dmask, u32 daddr, 
                           u32 smask, u32 saddr,
                           u64 _sel_mask, u64 _selector, u8 _proto)
{
  Eroute **r = &first;
  while (*r && !(*r)->equals(src_if, dmask, daddr, smask, saddr, 
	                     _sel_mask, _selector, _proto))
    r = (*r)->chain();
  if (!*r)
    return ENOTFND;
  Eroute *next = *(*r)->chain();
  delete *r;
  *r = next;
  return EOK;
}

unsigned Eroute_table::update_sa(SA *new_sa)
{
  Eroute *r = first;
  while (r)
    {
      if (r->re()->is_void() && r->re()->satype() == new_sa->satype() 
	  && r->re()->dst() == new_sa->dst())
	{
	  Base_sa *sa = r->re();
	  r->re(new_sa);
	  delete sa;
	}
      r = *(r->chain());
    }
  
  return EOK;

}
  
unsigned Eroute_table::void_all()
{
  Eroute *r = first;
  while (r)
    {
      if (!r->re()->is_void())
	{
	  SA *sa = r->sa();
	  r->re(new Void_sa(sa));
	}
      r = *(r->chain());
    }
  
  return EOK;
}

unsigned Eroute_table::void_all(u32 type, u32 spi, u32 dst)
{
  Eroute *r = first;
  while (r)
    {
      if (!r->re()->is_void() && r->sa()->equals(type,spi,dst))
	{
	  SA *sa = r->sa();
	  r->re(new Void_sa(sa));
	}
      r = *(r->chain());
    }
  
  return EOK;
}

void Eroute_table::flush()
{
  Eroute *r = first;
  _if_mode = 0;
  first = 0;
  while (r)
    {
      Eroute *n = *r->chain();
      delete r;
      r = n;
    }
}

void Eroute_table::print(L4::BasicOStream &o) const
{
  Eroute *r = first;
  while (r)
    {
      r->print(o); o << '\n';
      r = *r->chain();
    }
}
  

void SA::expired(Expire e) const
{
  bool h = false;
  
  if (e == hard_lt)
    h = true;

  L4::cout << "SA (";
  print(L4::cout);
  L4::cout << "): EXPIRED, queue msg\n";
  _ike_c->expired(this, h);

}

void SA::print(L4::BasicOStream &o) const
{
  L4::IOBackend::Mode m = o.be_mode();

  switch (type())
    {
    case ah: o << "ah/"; break;
    case esp: o << "esp/"; break;
    case pas: o << "pas/"; break;
    default:  o << "BUG(" << type() << ")/"; break;
    }
  o << "0x" << L4::hex << Net::n_to_h(_spi) << '/'
    << Ip_addr(dst(), (u32)-1) << ((satype() & tun)?"/tunnel":"/transport");
  o.be_mode(m);
}

void SA::print_full(L4::BasicOStream &o) const
{
  print(o); o << ' '; 
  if (c)
    c->print(o);
  if (a && c)
    o << '-';
  if (a) 
    a->print(o); 
  if (i)
    o << " ipip(s=" << Ip_addr(i->saddr(),(u32)-1) << ")";
  
  o << " -> "
    << "0x" << L4::hex << iface();
}

  
void Eroute::print(L4::BasicOStream &o) const
{
  o << Ip_addr(s_addr,s_mask) << " -> "
    << Ip_addr(d_addr,d_mask) << " via ";
  re()->print(o);
}

void Void_sa::print(L4::BasicOStream &o) const
{
  L4::IOBackend::Mode m = o.be_mode();

  switch (type())
    {
    case ah: o << "VOID(ah/"; break;
    case esp: o << "VOID(esp/"; break;
    case pas: o << "VOID(pas/"; break;
    default:  o << "VOID(BUG(" << type() << ")/"; break;
    }
  o << Ip_addr(dst(), (u32)-1) << ((satype() & tun)?"/tunnel)":"/transport)");
  o.be_mode(m);
}
