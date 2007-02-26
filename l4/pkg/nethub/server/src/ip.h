/* -*- c++ -*- */

/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef IP_PKT_H__
#define IP_PKT_H__

#include "types.h"
#include "net.h"

namespace L4
{
  class BasicOStream;
};

class Ip_addr
{
public:
  Ip_addr( u32 a=0, u32 m=0 ) : a(a), m(m) {}

  u32 a;
  u32 m;
};

L4::BasicOStream &operator << (L4::BasicOStream &s, Ip_addr const &a);

class Ip_packet
{
public:

  enum {
    IP_IP_PROTO = 0x04,
    /* IP flags. */
    IP_CE     = 0x8000,	/* Flag: "Congestion"		*/
    IP_DF     = 0x4000,	/* Flag: "Don't Fragment"	*/
    IP_MF     = 0x2000,	/* Flag: "More Fragments"	*/
    IP_OFFSET = 0x1FFF,	/* "Fragment Offset" part	*/
  };

  u8 head_len() const
  { return _ihl_version & 0x0f; }

  u8 version() const
  { return _ihl_version >> 4; }

  u8 protocol() const
  { return _protocol; }

  void protocol( u8 p )
  { _protocol = p; }

  u32 daddr() const
  { return _daddr; }
  
  u32 saddr() const
  { return _saddr; }

  u16 len() const
  { return Net::n_to_h(_tot_len); }

  void len( u16 l )
  { _tot_len = Net::h_to_n(l); }

  u8 ttl() const
  { return _ttl; }

  void ttl( u8 ttl ) 
  { _ttl = ttl; }
  
  u16 is_fragment() const 
  { return Net::n_to_h(_frag_off) & (IP_MF|IP_OFFSET); }

  u16 calc_checksum() const;

  bool check_checksum() const
  { return (calc_checksum() == 0xffff); }

  void set_checksum()
  {
    _check = 0; asm( "" : : :"memory");
    _check = ~(u16)calc_checksum();
  }

  u8 *pay_load()
  { return reinterpret_cast<u8*>(reinterpret_cast<u32*>(this)+head_len()); }

  void copy_header( Ip_packet *out ) const
  {
    for ( unsigned l = 0; l<head_len(); l++ )
      reinterpret_cast<u32*>(out)[l] = reinterpret_cast<u32 const*>(this)[l];
  }

  u16 id() const 
  { return _id; /* no n_to_h because it is superfluous*/ }

  void id( u16 i )
  { _id = i; }

  Ip_packet() {}
  Ip_packet( u8 version, u8 hlen, u8 tos, u16 len, u16 id, u16 foff,
             u8 ttl, u8 proto, u32 saddr, u32 daddr )
    : _ihl_version((hlen & 0x0f) | (version << 4)),
      _tos(tos), _tot_len(Net::h_to_n(len)), _id(id), 
      _frag_off(Net::h_to_n(foff)), _ttl(ttl), _protocol(proto),
      _saddr(saddr), _daddr(daddr)
  {}

private:
  u8    _ihl_version;
  u8	_tos;
  u16	_tot_len;
  u16	_id;
  u16	_frag_off;
  u8	_ttl;
  u8	_protocol;
  u16	_check;
  u32	_saddr;
  u32	_daddr;
  u8    _data[0];
};

#endif 

