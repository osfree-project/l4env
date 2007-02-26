/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "pager.h"

#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>
#include <l4/cxx/l4types.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>

#include "region.h"

Region const *Pager::find_region( unsigned long address ) const
{
  for (unsigned i=0 ; i< (sizeof(_r)/sizeof(_r[0])); ++i)
    {
      if (_r[i] && _r[i]->in_region(address))
	return _r[i];
    }
  return 0;
}
  
void Pager::add_region( Region *r )
{
  if (!r)
    return;
  
  for (unsigned i=0; i< (sizeof(_r)/sizeof(_r[0])); ++i)
    {
      if (_r[i] == 0)
	{
	  _r[i] = r;
//	  L4::cout << "pager: new region[" << i << "] = " << *_r[i] << "\n";
	  return;
	}
    }
  L4::cerr << "waring: pager: no more empty regions\n";
}

void Pager::del_region( Region *r )
{
  if (!r)
    return;
  
  for (unsigned i=0; i< (sizeof(_r)/sizeof(_r[0])); ++i)
    {
      if (_r[i] == r)
	{
	  _r[i] = 0;
	  return;
	}
    }
//  L4::cout << "pager: del_region(" << *r << "): region not found\n";
}

void Pager::run()
{
  L4::cout << '(' << self() << ") Pager is up!\n";
  while(1) 
    {
      l4_umword_t d1,d2;
      l4_msgdope_t result;
      l4_threadid_t t;
      void *desc;
      int err = l4_ipc_wait(&t, 0, &d1, &d2, L4_IPC_NEVER, &result);
      while(!err) 
	{
#if 0
	  L4::cout << "received d1=0x" << L4::hex << d1 
	    << ", d2=0x" << d2 << L4::dec 
	    << " from thread=" << t << "\n";
#endif
	  desc = L4_IPC_SHORT_MSG;

	  if (t.id.task != self().id.task) 
	    {
	      L4::cerr << "warning: pager: got message from other task, "
     		          "just don't answer\n";
	      break;
	    } 
	  else if (t.id.lthread < 9) 
	    {
#if 0
	      L4::cout << "pager: got message from non-hub thread, "
		"forward it\n";
#endif

	      if (d1 & 2) 
		l4_touch_rw((void*)(d1 & ~3), 4);
	      else
		l4_touch_ro((void*)(d1 & ~3), 4);

	      d1 = d2 = 0;
	    } 
	  else 
	    {
#if 0
	      L4::cout << "pager: hub (" << t << ") pf @" << L4::hex << (d1 & ~3)
		<< " ip=" << d2 << "\n";
#endif
	      Region const *r = find_region(d1 & ~3);
	      if (r) 
		{
#if 0
		  L4::cout << "pager: pf in comm region " << *r 
		    << "\n";
#endif
		  if (r->request_mapping( d1 & ~3 ))
		    r->unresolved_fault();
		}
	      else if (d1 & 2) 
		l4_touch_rw((void*)(d1 & ~3), 4);
	      else
		l4_touch_ro((void*)(d1 & ~3), 4);

	      d1 = d2 = 0;
	    }

	  err = l4_ipc_reply_and_wait(t, desc, d1, d2, 
	      &t, 0, &d1, &d2,
	      L4_IPC_SEND_TIMEOUT_0, 
	      /* snd timeout = 0 */
	      &result);
	}
    }
}

Pager::Pager() 
  : Thread(stack + sizeof(stack) -sizeof(l4_umword_t))
{}

