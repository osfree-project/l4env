/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef NH_ROUTING_H__
#define NH_ROUTING_H__

class Tx_iface;
class Ip_packet;

class Routing_entry
{
public:
  virtual Tx_iface *destination() const = 0;
  virtual ~Routing_entry();
  
};

class Routing_table
{
public:
  virtual Routing_entry *route(unsigned iface, Ip_packet *p) const = 0;
};

#endif // NH_ROUTING_H__

