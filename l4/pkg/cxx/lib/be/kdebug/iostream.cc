/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/iostream.h>
#include <l4/sys/kdebug.h>

namespace L4 {

  class KdbgIOBackend : public IOBackend
  {
  protected:
    void write(char const *str, unsigned len);
  };
  
  void KdbgIOBackend::write(char const *str, unsigned len)
  {
    outnstring(str,len);
  }

  namespace {
    KdbgIOBackend iob;
  };
  
  BasicOStream cout(&iob);
  BasicOStream cerr(&iob);
};
