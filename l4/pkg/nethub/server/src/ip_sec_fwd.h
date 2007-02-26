/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_IP_SEC_FWD_H__
#define L4_NH_IP_SEC_FWD_H__

#include "ip_forward.h"
#include "types.h"
#include "net.h"
#include "sadb.h"

namespace L4 {
  class BasicOStream;
}

class Ip_ipsec_fwd : public Ip_fwd
{
public:
  Ip_ipsec_fwd(Iface *out) : Ip_fwd(out) {}
  Fwd_result forward( Routing_entry *re, Ip_packet *in, Ip_packet *out );
  Fwd_result do_ipsec_forward( Routing_entry *re, Ip_packet *in, Ip_packet *out);
  Fwd_result esp_forward( Routing_entry *re, Ip_packet *in, Ip_packet *out );
  Fwd_result ah_forward( Routing_entry *re, Ip_packet *in, Ip_packet *out );
  void print( L4::BasicOStream &s ) const;
};

#endif // L4_NH_IP_SEC_FWD_H__

