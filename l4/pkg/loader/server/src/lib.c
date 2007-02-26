/* $Id$ */
/**
 * \file	loader/server/src/lib.c
 * \brief	handling of loading dynamic libraries
 *
 * \date 	05/2004
 * \author	Frank Mehnert */

/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/exec/exec.h>

#include "app.h"
#include "lib.h"
#include "dm-if.h"
#include "fprov-if.h"
#include "exec-if.h"

int
lib_load(app_t *app, const char *fname, l4_threadid_t fprov_id)
{
  int error;

  if ((error = exec_if_open(app, fname, &L4DM_INVALID_DATASPACE, 
			    L4EXEC_LOAD_LIB)))
    return error;

  return 0;
}

int
lib_link(app_t *app)
{
  return exec_if_link(app);
}
