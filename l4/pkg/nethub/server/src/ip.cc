/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "ip.h"
#include <l4/cxx/iostream.h>

u16 Ip_packet::calc_checksum() const
{
  u32 sum = 0;
  u16 const *pos = reinterpret_cast<u16 const*>(this);
  for( unsigned l = head_len()*2; l>0; --l) {
    sum += (*pos++); 
    if(sum & 0x80000000) 
      sum = (sum & 0xffff) + (sum>>16);
  }
  while(sum>>16)
    sum = (sum & 0xffff) + (sum>>16);

  return sum;
}

L4::BasicOStream &operator << (L4::BasicOStream &s, Ip_addr const &a)
{
  L4::IOBackend::Mode m = s.be_mode();
  u32 _a = Net::n_to_h(a.a);
  s << L4::dec << (_a>>24) << '.' << ((_a>>16) & 0xff) << '.'
    << ((_a>>8) & 0xff) << '.' << (_a & 0xff);
 
#if 0
  u32 _m = Net::n_to_h(a.m);
  if (a.m != (u32)-1)
    s << '/' << (_m>>24) << '.' << ((_m>>16) & 0xff) << '.'
      << ((_m>>8) & 0xff) << '.' << (_m & 0xff);
#else
  int i = 31;
  u32 _m = Net::n_to_h(a.m);
  for(;(_m & (1<<i)) && i>=0;--i);
  if (i != -1)
    s << '/' << (31-(unsigned)i);
#endif

  s.be_mode(m);
  return s;
}
