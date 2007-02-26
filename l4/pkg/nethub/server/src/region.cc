/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "region.h"

#include <l4/cxx/l4types.h>
#include <l4/cxx/iostream.h>
#include <l4/cxx/l4iostream.h>

#include <l4/sys/ipc.h>
#include <l4/sys/types.h>

L4::BasicOStream &operator << (L4::BasicOStream &o, Region const &r)
{
  if (r.start() != r.end()) 
    {
      o << "[0x" << L4::hex << (unsigned)r.start() << " - 0x" << 
	(unsigned)(r.end()-1) << "(0x" << (unsigned)r.offset() <<")]";
    }
  else
    o << "[empty]";
  return o;
}

int Region::request_mapping( unsigned long address ) const
{
  l4_umword_t w1,w2;
  l4_msgdope_t result;
  int err;
  l4_fpage_t fp = l4_fpage( address, 12, L4_FPAGE_RW, L4_FPAGE_MAP );
  w2 = address - _start + _offset;
  w1 = 0xaffec00d;

  if (address == last_request)
    return -2;

  last_request = address;

  err = l4_ipc_call( _mapper, 0, w1, w2, 
                     (void*)(fp.raw | 2), &w1, &w2,
                     L4_IPC_TIMEOUT(153,7,153,7,0,0),
		     &result );

  L4::MsgDope res(result);
  
  if (err)
    {
      L4::cout << "ERROR while requesting page from mapper: " 
	       << res << "\n";
      return err;
    }

  if ( !res.fpage_received() )
    {
      L4::cout << "ERROR no mapping received from mappper\n";
      return -1;
    }
#if 0
  L4::cout << "got mapping from " << _mapper << ": " << L4::hex 
           << w2 << "\n"; 
#endif
  
  return 0;
}
