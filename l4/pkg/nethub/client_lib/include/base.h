/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_NH_BASE_H__
#define L4_NH_BASE_H__

#include <l4/sys/types.h>
#include <l4/nethub/base-types.h>

#ifdef __cplusplus
extern "C" {
#endif

Hub nh_resolve_hub(char const *name);

#ifdef __cplusplus
}
#endif

#endif // L4_NH_BASE_H__

