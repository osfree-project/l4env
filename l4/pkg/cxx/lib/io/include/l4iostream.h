/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_L4IOSTREAM_H__
#define L4_L4IOSTREAM_H__

#include <l4/cxx/iostream.h>
#include <l4/sys/types.h>

inline L4::BasicOStream &operator << ( L4::BasicOStream &s, l4_threadid_t id )
{
  L4::IOBackend::Mode m = s.be_mode();
  s << L4::hex << id.id.task << "." << id.id.lthread;
  s.be_mode(m);
  return s;
}


#endif
