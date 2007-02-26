/*!
 * \file	loader/server/src/lib.h
 * \brief	handling of loading dynamic libraries
 *
 * \date	05/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __LIB_H_
#define __LIB_H_

#include <l4/env/env.h>

int lib_load(app_t *app, const char *fname, l4_threadid_t fprov_id);
int lib_link(app_t *app);

#endif

