/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "ip_sec_fwd.h"
#include "ip.h"

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>

Ip_fwd::Fwd_result 
Ip_ipsec_fwd::do_ipsec_forward(Routing_entry *re, 
                               Ip_packet *in, Ip_packet *out)
{
  unsigned max_len = out->len();
  unsigned in_len  = in->len();

  SA *x = (SA*)re;

  SA::Expire cause;
  
  switch (x->type())
    {
    case SA::pas: 
      if ((cause = x->send_bytes(in->len()))!=SA::no_lt)
	x->expired(cause);
      return Ip_fwd::forward(re, in, out);
      
    case SA::esp:
      {
	if (!x->c)
	  {
	    L4::cout << "Drop IP packet due to encryption error\n";
	    return fwd_drop;
	  }

	if (max_len < (unsigned)in->head_len()*4)
	  return fwd_out_size_error;

	unsigned long p_len;
	void *e_data;

	if (x->i)
	  {
	    x->i->copy_header(out);
	    x->i->id(x->i->id() + 1);
	    p_len = in_len;
	    e_data = in;
	    in_len += x->i->head_len()*4;
	  }
	else
	  {
	    in->copy_header(out);
	    p_len = in_len - in->head_len()*4;
	    e_data = in->pay_load();
	  }

	in_len += 32 + 8; // buffer space for enc + ESP head

	if (max_len < in_len)
	  {
	    out->len(in_len);
	    return fwd_out_size_error;
	  }

	char np = out->protocol();

	out->protocol(IP_ESP_PROTO);
	Ip_esp_packet *e_out = static_cast<Ip_esp_packet*>(out);
	Ip_esp_packet::Esp_h *esp_head = e_out->get_esp_head();
	esp_head->spi(x->spi());
	esp_head->seq(x->seq++);

	x->c->encrypt(p_len, e_data, esp_head->data, np);

	p_len += 8; // spi + seq

	if (x->a) 
	  x->a->auth(p_len, esp_head, true);

	out->len(out->head_len()*4 + p_len);
	out->set_checksum();

	if ((cause = x->send_bytes(in_len)) != SA::no_lt)
	  x->expired(cause);

	return fwd_ok;
      }
      
    case SA::ah:
    default:
      return fwd_drop;
    }
}

Ip_fwd::Fwd_result 
Ip_ipsec_fwd::esp_forward(Routing_entry *re, Ip_packet *in, Ip_packet *out)
{
  Ip_esp_packet *e_in = static_cast<Ip_esp_packet*>(in);
  Ip_esp_packet::Esp_h *esp_head = e_in->get_esp_head();
  
  SA *x = (SA*)re;
  
  if (!x)
    {
      L4::cout << self() << ": No SA for esp0x" << L4::hex 
	       << Net::n_to_h(e_in->spi())
	       << '@' << Ip_addr(e_in->daddr(), (u32)-1) << '\n';
      return fwd_retry;
    }

#if 0
  L4::cout << self() << ": use esp0x" << L4::hex << Net::n_to_h(x->spi()) 
           << '@' << Ip_addr(x->dst(), 0xffffffff ) << '\n';
#endif
  
  unsigned in_len = in->len();
  unsigned out_len = out->len();
  unsigned long p_len = in_len - in->head_len()*4;
  
  if (!x->a || !x->a->auth(p_len, esp_head, false))
    {
      L4::cout << "Drop ESP packet due to authentication error\n";
      return fwd_drop; // drop
    }

  p_len -= 8; // take away spi and seq

  void *e_out;
 
  if (!x->i)
    {
      in->copy_header(out);
      e_out = out->pay_load();
    }
  else
    {
      in_len -= in->head_len()*4;
      e_out = out;
    }

  if (out_len < in_len)
    {
      out->len(in_len);
      return fwd_out_size_error;
    }
  
  if (x->c)
    x->c->decrypt(p_len, esp_head->data, e_out);
 
  if (!x->i)
    {
      out->len(out->head_len()*4 + p_len);
      Ip_esp_packet *e_out = static_cast<Ip_esp_packet*>(out);
      e_out->un_esp();
    }

  SA::Expire cause;
  
  if ((cause = x->send_bytes(p_len)) != SA::no_lt)
    x->expired(cause);

  return fwd_ok;
}

Ip_fwd::Fwd_result Ip_ipsec_fwd::ah_forward(Routing_entry *re, 
                                            Ip_packet *in, Ip_packet *out)
{
  L4::cout << "Got AH packet\n"
              " - policy is drop\n";
  return fwd_retry;
}

Ip_fwd::Fwd_result 
Ip_ipsec_fwd::forward(Routing_entry *re, Ip_packet *in, Ip_packet *out)
{
  SA *s = (SA*)re;

  if (s->direction == SA::dir_out)
    {
      // L4::cout << "process outbound packet\n";
      return do_ipsec_forward(re, in, out);
    }
  
  if(in->is_fragment()) 
    {
      L4::cout << "IPsec can't handle fragments\n";
      return fwd_drop;
    }
 
  SA::Expire cause;
  
  switch(s->type()) 
    {
    case SA::esp:
      return esp_forward(re, in, out);
    case SA::ah:
      return ah_forward(re, in, out);
    case SA::pas:
      if ((cause = s->send_bytes(in->len())) != SA::no_lt)
	s->expired(cause);
      return Ip_fwd::forward( re, in, out );
    default: /*drop*/
      return fwd_drop;
  };
}

void Ip_ipsec_fwd::print(L4::BasicOStream &s) const
{
  s << "Ip_ipsec_fwd: decryptor + authenticator with SPD";
}

