/* -*- c++ -*- */

/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef IP_FORWARD_H__
#define IP_FORWARD_H__

#include <l4/cxx/thread.h>
#include <l4/sys/types.h>
//#include <l4/nethub/nethub.h>

#include "interface.h"
#include "ids.h"
#include "ip.h"
// prevent oskit10 from defining wchar_t
#define _WCHAR_T
#include <stddef.h>
#if 0
#include <l4/cxx/iostream.h>
#endif

namespace L4 {
  class BasicOStream;
}

class Iface;

class Ip_fwd : public L4::Thread
{
private:
  L4_uid _rcv;
  L4_threadid _snd;
  l4_umword_t stack[1024];
  bool test_checksum;
  Iface *out_iface;

public:

  enum Fwd_result {
    fwd_ok = 0,
    fwd_drop = 1,
    fwd_out_size_error = 2,
    fwd_retry = 3,
  };

  Ip_fwd( Iface *out );
  virtual ~Ip_fwd();
  void run();
  void rcv( L4_uid i ) { _rcv = i; }
  void snd( L4_threadid const &i ) { _snd = i; }
  
  bool is_ready() 
  { 
    return _rcv.is_valid() && _snd.is_valid(); 
  }

  bool has_sender() { return _snd.is_valid(); }
  bool has_receiver() { return _rcv.is_valid(); }

  virtual Fwd_result forward( Routing_entry *re, Ip_packet *in, Ip_packet *out );
  virtual char const *const name() const;
  virtual void print( L4::BasicOStream &s ) const;  
};

inline L4::BasicOStream &operator << (L4::BasicOStream &s, Ip_fwd const &f)
{
  f.print(s);
  return s;
}

#endif  //IP_FORWARD_H__

