/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/iostream.h>

void operator delete (void *obj)
{
  L4::cerr << "cxa pure delete operator called for object @" 
           << L4::hex << obj << L4::dec << "\n";
}
