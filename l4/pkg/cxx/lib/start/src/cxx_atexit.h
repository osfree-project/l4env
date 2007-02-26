/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the cxx package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef L4_CXX_ATEXIT_H__
#define L4_CXX_ATEXIT_H__

extern "C" void __cxa_finalize(void *dso_handle);
extern "C" int __cxa_atexit(void (*f)(void*), void *arg, void *dso_handle);
extern "C" int atexit(void (*f)(void));

#endif
