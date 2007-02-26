/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef NH_ROUTING_FAB_H__
#define NH_ROUTING_FAB_H__

#include <l4/cxx/thread.h>
#include "routing.h"
#include "region.h"

class Iface_list;

class Routing_fab : public L4::Thread, public Region_handler
{
public:
  Routing_fab( Routing_table *rtab, Iface_vector *ifl ) 
    : L4::Thread( stack + 1023, 9 ), rtab(rtab), ifl(ifl)
  {}
  
  void run();

  void unresolved_fault( void *priv );
      
private:
  Routing_table *rtab;  
  Iface_vector *ifl;
  unsigned long stack[1024];
};

#endif // NH_ROUTING_FAB_H__

