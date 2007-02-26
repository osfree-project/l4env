/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/cxx/main_thread.h>
#include <l4/crtx/crt0.h>
#include <l4/cxx/base.h>

#include "cxx_atexit.h"

extern L4::MainThread *main;

extern "C" 
void __main()
{
  crt0_construction();
  //  __cxa_atexit( crt0_destruction );
  if(main)
    main->run();
  __cxa_do_atexit();
  crt0_sys_destruction();
  l4_sleep_forever();
}
