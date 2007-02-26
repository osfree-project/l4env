/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/base.h>
#include <l4/cxx/iostream.h>

extern "C" 
void abort(void) __attribute((noreturn));

void abort(void) 
{
  L4::cerr << "Aborted\n";
  l4_sleep_forever();
}
