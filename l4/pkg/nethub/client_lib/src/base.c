/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/nethub/base.h>
#include <l4/names/libnames.h>

Hub nh_resolve_hub(char const *name)
{
  l4_threadid_t hub = L4_INVALID_ID;
  names_waitfor_name(name, &hub, 5000);
  return hub;
}

