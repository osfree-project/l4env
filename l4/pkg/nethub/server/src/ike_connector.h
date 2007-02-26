/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_IKE_CONNECTOR_H__
#define L4_NH_IKE_CONNECTOR_H__

#include <l4/cxx/thread.h>
#include <l4/nethub/cfg-types.h>

class Ip_packet;
class SA;

class Ike_notify_buffer
{
public:
  union Element 
  {
    Nh_sadb_expire_msg exp;
    Nh_sadb_acquire_msg acq;
  };
  
  Ike_notify_buffer(); 

  Element *get_element();
  bool commit_element(Element *, unsigned type);
  
private:

  enum
  {
    Num_elements = 85,
  };
  
  Element _buf[Num_elements];
  Element __attribute__((aligned(4096))) *free;
} __attribute__((aligned(4096)));

class Ike_connector : public L4::Thread
{
public:
  Ike_connector(char const *name) 
  : Thread(stack+1023), _name(name)
  {}

  void run();

  bool acquire(unsigned satype, Ip_packet const *p, unsigned iface);
  bool expired(SA const *sa, bool hard);

  Ike_notify_buffer *buffer() 
  { return &_nbuf; }

private:
  Ike_notify_buffer _nbuf;
  l4_umword_t stack[1024];
  char const *_name;
};

#endif // L4_NH_IKE_CONNECTOR_H__

