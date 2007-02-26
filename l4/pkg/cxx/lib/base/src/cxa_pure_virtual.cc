/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/iostream.h>

extern "C" void __cxa_pure_virtual()
{
  L4::cerr << "cxa pure virtual function called\n";
}


extern "C" void __pure_virtual()
{
  L4::cerr << "cxa pure virtual function called\n";
}
