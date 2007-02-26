/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_BASE_TYPES_H__
#define L4_NH_BASE_TYPES_H__

#include <l4/sys/types.h>

typedef l4_threadid_t Hub;

/// Return values of SADB functions.
enum Nh_msg_error 
{
  NH_OK      = 1, ///< Well done
  NH_EINVAL  = 2, ///< Invalid argument
  NH_EEXISTS = 3, ///< Some Item already exists
  NH_ENOENT  = 4, ///< No such entry
  NH_EIPC    = 0, ///< IPC error
};

#endif // L4_NH_BASE_TYPES_H__

